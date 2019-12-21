/**************************************************************************/
/*!
    @file     Adafruit_LIS3MDL.cpp
    @author   Limor Fried (Adafruit Industries)

    This is a library for the Adafruit LIS3MDL magnetometer breakout board
    ----> https://www.adafruit.com

    Adafruit invests time and resources providing this open source code,
    please support Adafruit and open-source hardware by purchasing
    products from Adafruit!

    @section license License
    BSD license, all text here must be included in any redistribution.
*/
/**************************************************************************/

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <Adafruit_LIS3MDL.h>
#include <Wire.h>

/**************************************************************************/
/*!
    @brief  Instantiates a new LIS3MDL class
*/
/**************************************************************************/
Adafruit_LIS3MDL::Adafruit_LIS3MDL() {}

/*!
 *    @brief  Sets up the hardware and initializes I2C
 *    @param  i2c_address
 *            The I2C address to be used.
 *    @param  wire
 *            The Wire object to be used for I2C connections.
 *    @return True if initialization was successful, otherwise false.
 */
bool Adafruit_LIS3MDL::begin(uint8_t i2c_address, TwoWire *wire) {
  i2c_dev = new Adafruit_I2CDevice(i2c_address, wire);

  if (!i2c_dev->begin()) {
    return false;
  }

  // Check connection
  Adafruit_BusIO_Register chip_id =
      Adafruit_BusIO_Register(i2c_dev, LIS3MDL_REG_WHO_AM_I, 1);

  // make sure we're talking to the right chip
  if (chip_id.read() != 0x3D) {
    // No LIS3MDL detected ... return false
    return false;
  }

  reset();

  // set high quality performance mode
  setPerformanceMode(LIS3MDL_HIGHMODE);

  // 80Hz rate
  setDataRate(LIS3MDL_DATARATE_80_HZ);

  // lowest range
  setRange(LIS3MDL_RANGE_4_GAUSS);

  /*
  // enable all axes
  enableAxes(true, true, true);
  // 250Hz bw
  setBandwidth(MSA301_BANDWIDTH_250_HZ);
  setResolution(MSA301_RESOLUTION_14);
  */
  /*
  // DRDY on INT1
  writeRegister8(MSA301_REG_CTRL3, 0x10);

  // Turn on orientation config
  //writeRegister8(MSA301_REG_PL_CFG, 0x40);

  */
  /*
  for (uint8_t i=0; i<0x30; i++) {
    Serial.print("$");
    Serial.print(i, HEX); Serial.print(" = 0x");
    Serial.println(readRegister8(i), HEX);
  }
  */

  return true;
}

void Adafruit_LIS3MDL::reset(void) {
  Adafruit_BusIO_Register CTRL_REG2 =
      Adafruit_BusIO_Register(i2c_dev, LIS3MDL_REG_CTRL_REG2, 1);
  Adafruit_BusIO_RegisterBits resetbits =
      Adafruit_BusIO_RegisterBits(&CTRL_REG2, 2, 2);
  resetbits.write(0b11);
  delay(10);
}

/**************************************************************************/
/*!
  @brief  Read the XYZ data from the magnetometer and store in the internal
  x, y and z (and x_g, y_g, z_g) member variables.
*/
/**************************************************************************/

void Adafruit_LIS3MDL::read(void) {
  uint8_t buffer[6];

  Adafruit_BusIO_Register XYZDataReg =
      Adafruit_BusIO_Register(i2c_dev, LIS3MDL_REG_OUT_X_L, 6);
  XYZDataReg.read(buffer, 6);
  x = buffer[0];
  x |= buffer[1] << 8;
  y = buffer[2];
  y |= buffer[3] << 8;
  z = buffer[4];
  z |= buffer[5] << 8;

  lis3mdl_range_t range = getRange();
  float scale = 1; // LSB per gauss
  if (range == LIS3MDL_RANGE_16_GAUSS)
    scale = 1711;
  if (range == LIS3MDL_RANGE_12_GAUSS)
    scale = 2281;
  if (range == LIS3MDL_RANGE_8_GAUSS)
    scale = 3421;
  if (range == LIS3MDL_RANGE_4_GAUSS)
    scale = 6842;

  x_gauss = (float)x / scale;
  y_gauss = (float)y / scale;
  z_gauss = (float)z / scale;
}

/**************************************************************************/
/*!
    @brief  Gets the most recent sensor event, Adafruit Unified Sensor format
    @param  event Pointer to an Adafruit Unified sensor_event_t object that
   we'll fill in
    @returns True on successful read
*/
/**************************************************************************/
bool Adafruit_LIS3MDL::getEvent(sensors_event_t *event) {
  /* Clear the event */
  memset(event, 0, sizeof(sensors_event_t));

  event->version = sizeof(sensors_event_t);
  event->sensor_id = _sensorID;
  event->type = SENSOR_TYPE_MAGNETIC_FIELD;
  event->timestamp = millis();

  read();

  event->magnetic.x = x_gauss * 100; // microTesla per gauss
  event->magnetic.y = y_gauss * 100; // microTesla per gauss
  event->magnetic.z = z_gauss * 100; // microTesla per gauss

  return true;
}

/**************************************************************************/
/*!
    @brief  Gets the sensor_t device data, Adafruit Unified Sensor format
    @param  sensor Pointer to an Adafruit Unified sensor_t object that we'll
   fill in
*/
/**************************************************************************/
void Adafruit_LIS3MDL::getSensor(sensor_t *sensor) {
  /* Clear the sensor_t object */
  memset(sensor, 0, sizeof(sensor_t));

  /* Insert the sensor name in the fixed length char array */
  strncpy(sensor->name, "LIS3MDL", sizeof(sensor->name) - 1);
  sensor->name[sizeof(sensor->name) - 1] = 0;
  sensor->version = 1;
  sensor->sensor_id = _sensorID;
  sensor->type = SENSOR_TYPE_MAGNETIC_FIELD;
  sensor->min_delay = 0;
  sensor->max_value = 0;
  sensor->min_value = 0;
  sensor->resolution = 0;
}

/**************************************************************************/
/*!
    @brief Set the performance mode, LIS3MDL_LOWPOWERMODE, LIS3MDL_MEDIUMMODE,
    LIS3MDL_HIGHMODE or LIS3MDL_ULTRAHIGHMODE
    @param mode Enumerated lis3mdl_performancemode_t
*/
/**************************************************************************/
void Adafruit_LIS3MDL::setPerformanceMode(lis3mdl_performancemode_t mode) {
  // write xy
  Adafruit_BusIO_Register CTRL_REG1 =
      Adafruit_BusIO_Register(i2c_dev, LIS3MDL_REG_CTRL_REG1, 1);
  Adafruit_BusIO_RegisterBits performancemodebits =
      Adafruit_BusIO_RegisterBits(&CTRL_REG1, 2, 5);
  performancemodebits.write((uint8_t)mode);

  // write z
  Adafruit_BusIO_Register CTRL_REG4 =
      Adafruit_BusIO_Register(i2c_dev, LIS3MDL_REG_CTRL_REG4, 1);
  Adafruit_BusIO_RegisterBits performancemodezbits =
      Adafruit_BusIO_RegisterBits(&CTRL_REG4, 2, 2);
  performancemodezbits.write((uint8_t)mode);
}

/**************************************************************************/
/*!
    @brief Get the performance mode
    @returns Enumerated lis3mdl_performancemode_t, LIS3MDL_LOWPOWERMODE, 
    LIS3MDL_MEDIUMMODE, LIS3MDL_HIGHMODE or LIS3MDL_ULTRAHIGHMODE
*/
/**************************************************************************/
lis3mdl_performancemode_t Adafruit_LIS3MDL::getPerformanceMode(void) {
  Adafruit_BusIO_Register CTRL_REG1 =
      Adafruit_BusIO_Register(i2c_dev, LIS3MDL_REG_CTRL_REG1, 1);
  Adafruit_BusIO_RegisterBits performancemodebits =
      Adafruit_BusIO_RegisterBits(&CTRL_REG1, 2, 5);
  return (lis3mdl_performancemode_t)performancemodebits.read();
}

/**************************************************************************/
/*!
    @brief  Sets the data rate for the LIS3MDL (controls power consumption)
    from 0.625 Hz to 80Hz
    @param dataRate Enumerated lis3mdl_dataRate_t
*/
/**************************************************************************/
void Adafruit_LIS3MDL::setDataRate(lis3mdl_dataRate_t dataRate) {
  if (dataRate == LIS3MDL_DATARATE_155_HZ) {
    // set OP to UHP
    setPerformanceMode(LIS3MDL_ULTRAHIGHMODE);
  }
  if (dataRate == LIS3MDL_DATARATE_300_HZ) {
    // set OP to HP
    setPerformanceMode(LIS3MDL_HIGHMODE);
  }
  if (dataRate == LIS3MDL_DATARATE_560_HZ) {
    // set OP to MP
    setPerformanceMode(LIS3MDL_MEDIUMMODE);
  }
  if (dataRate == LIS3MDL_DATARATE_1000_HZ) {
    // set OP to LP
    setPerformanceMode(LIS3MDL_LOWPOWERMODE);
  }
  delay(10);
  Adafruit_BusIO_Register CTRL_REG1 =
      Adafruit_BusIO_Register(i2c_dev, LIS3MDL_REG_CTRL_REG1, 1);
  Adafruit_BusIO_RegisterBits dataratebits =
      Adafruit_BusIO_RegisterBits(&CTRL_REG1, 4, 1); // includes FAST_ODR
  dataratebits.write((uint8_t)dataRate);

}

/**************************************************************************/
/*!
    @brief  Gets the data rate for the LIS3MDL (controls power consumption)
    @return Enumerated lis3mdl_dataRate_t from 0.625 Hz to 80Hz
*/
/**************************************************************************/
lis3mdl_dataRate_t Adafruit_LIS3MDL::getDataRate(void) {
  Adafruit_BusIO_Register CTRL_REG1 =
      Adafruit_BusIO_Register(i2c_dev, LIS3MDL_REG_CTRL_REG1, 1);
  Adafruit_BusIO_RegisterBits dataratebits =
    Adafruit_BusIO_RegisterBits(&CTRL_REG1, 4, 1); // includes FAST_ODR
  return (lis3mdl_dataRate_t)dataratebits.read();
}


/**************************************************************************/
/*!
    @brief Set the operation mode, LIS3MDL_CONTINUOUSMODE,
    LIS3MDL_SINGLEMODE or LIS3MDL_POWERDOWNMODE
    @param mode Enumerated lis3mdl_operationmode_t
*/
/**************************************************************************/
void Adafruit_LIS3MDL::setOperationMode(lis3mdl_operationmode_t mode) {
  // write x and y
  Adafruit_BusIO_Register CTRL_REG3 =
      Adafruit_BusIO_Register(i2c_dev, LIS3MDL_REG_CTRL_REG3, 1);
  Adafruit_BusIO_RegisterBits opmodebits =
      Adafruit_BusIO_RegisterBits(&CTRL_REG3, 2, 0);
  opmodebits.write((uint8_t)mode);
}

/**************************************************************************/
/*!
    @brief Get the operation mode
    @returns Enumerated lis3mdl_operationmode_t, LIS3MDL_CONTINUOUSMODE,
    LIS3MDL_SINGLEMODE or LIS3MDL_POWERDOWNMODE
*/
/**************************************************************************/
lis3mdl_operationmode_t Adafruit_LIS3MDL::getOperationMode(void) {
  Adafruit_BusIO_Register CTRL_REG3 =
      Adafruit_BusIO_Register(i2c_dev, LIS3MDL_REG_CTRL_REG3, 1);
  Adafruit_BusIO_RegisterBits opmodebits =
      Adafruit_BusIO_RegisterBits(&CTRL_REG3, 2, 0);
  return (lis3mdl_operationmode_t)opmodebits.read();
}

/**************************************************************************/
/*!
    @brief Set the resolution range: +-4 gauss, 8 gauss, 12 gauss, or 16 gauss.
    @param range Enumerated msa301_range_t
*/
/**************************************************************************/
void Adafruit_LIS3MDL::setRange(lis3mdl_range_t range) {
  Adafruit_BusIO_Register CTRL_REG2 =
      Adafruit_BusIO_Register(i2c_dev, LIS3MDL_REG_CTRL_REG2, 1);
  Adafruit_BusIO_RegisterBits rangebits =
      Adafruit_BusIO_RegisterBits(&CTRL_REG2, 2, 5);
  rangebits.write((uint8_t)range);
}

/**************************************************************************/
/*!
    @brief Read the resolution range: +-4 gauss, 8 gauss, 12 gauss, or 16 gauss.
    @returns Enumerated lis3mdl_range_t
*/
/**************************************************************************/
lis3mdl_range_t Adafruit_LIS3MDL::getRange(void) {
  Adafruit_BusIO_Register CTRL_REG2 =
      Adafruit_BusIO_Register(i2c_dev, LIS3MDL_REG_CTRL_REG2, 1);
  Adafruit_BusIO_RegisterBits rangebits =
      Adafruit_BusIO_RegisterBits(&CTRL_REG2, 2, 5);
  return (lis3mdl_range_t)rangebits.read();
}
