idf_component_register(
  SRCS "touch_bsp.c"
  REQUIRES esp_driver_i2c esp_driver_gpio
  INCLUDE_DIRS "include")
