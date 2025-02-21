
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/iio/iio.h>

#define LTC2607_DRV_NAME "ltc2607"

struct ltc2607_device {
	struct i2c_client *client;
	char name[8];
};

static const struct iio_chan_spec ltc2607_channel[] = {
{
	.type		= IIO_VOLTAGE,
	.indexed	= 1,
	.output		= 1,
	.channel	= 0,
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
},{
	.type		= IIO_VOLTAGE,
	.indexed	= 1,
	.output		= 1,
	.channel	= 1,
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
},{
	.type		= IIO_VOLTAGE,
	.indexed	= 1,
	.output		= 1,
	.channel	= 2,
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
}

};

static int ltc2607_set_value(struct iio_dev *indio_dev, int val, int channel)
{
	struct ltc2607_device *data = iio_priv(indio_dev);
	u8 outbuf[3];
	int ret;
	int chan;

	if (channel == 2)
		chan = 0x0F;
	else
		chan = channel;

	if (val >= (1 << 16) || val < 0)
		return -EINVAL;

	outbuf[0] = 0x30 | chan; /* write and update DAC */
	outbuf[1] = (val >> 8) & 0xff; /* MSB byte of dac_code */
	outbuf[2] = val & 0xff; /* LSB byte of dac_code */

	ret = i2c_master_send(data->client, outbuf, 3);
	if (ret < 0)
		return ret;
	else if (ret != 3)
		return -EIO;
	else
		return 0;
}

static int ltc2607_write_raw(struct iio_dev *indio_dev,
			       struct iio_chan_spec const *chan,
			       int val, int val2, long mask)
{
	int ret;

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		ret = ltc2607_set_value(indio_dev, val, chan->channel);
		return ret;
	default:
		return -EINVAL;
	}
}

static const struct iio_info ltc2607_info = {

	.write_raw = ltc2607_write_raw,
};

static int ltc2607_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	static int counter = 0;
	struct iio_dev *indio_dev;
	struct ltc2607_device *data;
	u8 inbuf[3];
	u8 command_byte;
	int err;
	dev_info(&client->dev, "DAC_probe()\n");
	
	command_byte = 0x30 | 0x00; /* Write and update register with value 0xFF*/
	inbuf[0] = command_byte;
	inbuf[1] = 0xFF;
	inbuf[2] = 0xFF;

	/* allocate the iio_dev structure */
	indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*data));
	if (indio_dev == NULL) 
		return -ENOMEM;
	
	data = iio_priv(indio_dev);	
	i2c_set_clientdata(client, data);
	data->client = client;
	
	sprintf(data->name, "DAC%02d", counter++);
	dev_info(&client->dev, "data_probe is entered on %s\n", data->name);

	indio_dev->name = data->name;
	indio_dev->dev.parent = &client->dev;
	indio_dev->info = &ltc2607_info;
	indio_dev->channels = ltc2607_channel;
	indio_dev->num_channels = 3;
	indio_dev->modes = INDIO_DIRECT_MODE;

	err = i2c_master_send(client, inbuf, 3); /* write DAC value */
	if (err < 0) {
		dev_err(&client->dev, "failed to write DAC value");
		return err;
	}
	
	dev_info(&client->dev, "the dac answer is: %x.\n", err);
	
	err = devm_iio_device_register(&client->dev, indio_dev);
	if (err)
		return err;

	dev_info(&client->dev, "ltc2607 DAC registered\n");

	return 0;
}

static int ltc2607_remove(struct i2c_client *client)
{
	dev_info(&client->dev, "DAC_remove()\n");
	return 0;
}

static const struct of_device_id dac_dt_ids[] = {
	{ .compatible = "arrow,ltc2607", },
	{ }
};
MODULE_DEVICE_TABLE(of, dac_dt_ids);

static const struct i2c_device_id ltc2607_id[] = {
	{ "ltc2607", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ltc2607_id);

static struct i2c_driver ltc2607_driver = {
	.driver = {
		.name	= LTC2607_DRV_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = dac_dt_ids,
	},
	.probe		= ltc2607_probe,
	.remove		= ltc2607_remove,
	.id_table	= ltc2607_id,
};
module_i2c_driver(ltc2607_driver);

MODULE_AUTHOR("Alberto Liberal <aliberal@arroweurope.com>");
MODULE_DESCRIPTION("LTC2607 16-bit DAC");
MODULE_LICENSE("GPL");
