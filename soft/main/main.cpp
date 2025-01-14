#include <nvs_flash.h>

#include <string.h>

#include <platform.h>

#include <app/controllers/attitude_controller.h>
#include <app/controllers/height_controller.h>
#include <app/controllers/position_controller.h>
#include <app/controllers/rate_controller.h>
#include <app/controllers/nmea_controller.h>
#include <app/data_recorder/data_recorder.h>
#include <app/workers/battery_supervisor.h>
#include <app/workers/camera_controller.h>

#include <data_model/data_ressources_registry.h>
#include <data_model/json_protocol.h>

#include <periph/i2c_master.h>
#include <periph/uart.h>

#include <hal/motor.h>
#include <hal/udp_server.h>
#include <hal/wifi.h>
#include <hal/marg.h>
#include <hal/log.h>

#include <os/task.h>

#include <utils/mixer.h>

// #define DATA_RECORDING

extern "C" void app_main(void)
{
    Logger                  * logger;
    Motor                   * front_left;
    Motor                   * front_right;
    Motor                   * rear_left;
    Motor                   * rear_right;
    I2cMaster               * sensors_i2c;
    Marg                    * marg;
    Uart                    * uart_1;
#ifdef DATA_RECORDING
    DataRecorder            * data_recorder;
#else
    Mixer                   * mixer;
    RateController          * rate_controller;
    AttitudeController      * attitude_controller;
    HeightController        * height_controller;
    PositionController      * position_controller;
    NMEA_controller         * nmea_controller;
    Wifi                    * wifi;
    UdpServer               * udp;
    DataRessourcesRegistry  * registry;
    JsonDataProtocol        * protocol;
    BatterySupervisor       * battery;
    CameraController        * camera;
    UltrasoundSensor        * ultrasound;
    Barometer               * barometer;
#endif

    nvs_flash_init();

    sensors_i2c = new I2cMaster(I2C_MASTER_NUM);
    sensors_i2c->init();

    uart_config_t uart_01_config = {
        .baud_rate              = 9600,
        .data_bits              = UART_DATA_8_BITS,
        .parity                 = UART_PARITY_DISABLE,
        .stop_bits              = UART_STOP_BITS_1,
        .flow_ctrl              = UART_HW_FLOWCTRL_DISABLE,
        .source_clk             = UART_SCLK_APB
    };
    uart_pin_config_t uart_01_pin_config = {
        .tx                     = GPIO_NUM_4,
        .rx                     = GPIO_NUM_5,
        .rts                    = UART_PIN_NO_CHANGE,
        .cts                    = UART_PIN_NO_CHANGE
    };
    uart_1 = new Uart(UART_NUM_1, uart_01_config, uart_01_pin_config);
    
    uart_pattern_t pattern_config = {
        .pattern_chr            = '\n',
        .chr_num                = 1,
        .chr_tout               = 9,
        .post_idle              = 0,
        .pre_idle               = 0,
    };
    uart_1->enable_pattern_detect(pattern_config);
    uart_1->register_pattern_detected_callback(NULL);

    uart_1->start_uart_event();


    marg = new Marg(sensors_i2c);
    marg->init();

    barometer = new Barometer(sensors_i2c);
    barometer->init();

    ultrasound = new UltrasoundSensor();

    front_left  = new Motor(PLATFORM_FRONT_LEFT_PULSE_CHANNEL,  PLATFORM_FRONT_LEFT_PULSE_PIN);
    front_right = new Motor(PLATFORM_FRONT_RIGHT_PULSE_CHANNEL, PLATFORM_FRONT_RIGHT_PULSE_PIN);
    rear_left   = new Motor(PLATFORM_REAR_LEFT_PULSE_CHANNEL,   PLATFORM_REAR_LEFT_PULSE_PIN);
    rear_right  = new Motor(PLATFORM_REAR_RIGHT_PULSE_CHANNEL,  PLATFORM_REAR_RIGHT_PULSE_PIN);

    front_left->arm();
    front_right->arm();
    rear_left->arm();
    rear_right->arm();

    front_left->set_speed(0.0f);
    front_right->set_speed(0.0f);
    rear_left->set_speed(0.0f);
    rear_right->set_speed(0.0f);

#ifdef DATA_RECORDING
    data_recorder       = new DataRecorder(front_left, front_right, rear_left, rear_right, marg);
#else
    registry            = new DataRessourcesRegistry("data_model.json");
    wifi                = new Wifi(registry);
    udp                 = new UdpServer("quadcopter_control", 5000);
    logger              = new Logger(udp);
    protocol            = new JsonDataProtocol(udp, registry);
    mixer               = new Mixer(front_left, front_right, rear_left, rear_right);
    rate_controller     = new RateController(RATE_CONTROLLER_PERIOD, marg, mixer);
    attitude_controller = new AttitudeController(ATTITUDE_CONTROLLER_PERIOD, registry, rate_controller, marg);
    height_controller   = new HeightController(HEIGHT_CONTROLLER_PERIOD, registry, marg, barometer, ultrasound, attitude_controller, rate_controller);
    position_controller = new PositionController(POSITION_CONTROLLER_PERIOD, registry);
    nmea_controller     = new NMEA_controller(registry);
    battery             = new BatterySupervisor(BATTERY_SUPERVISOR_PERIOD, registry);
    camera              = new CameraController(CAMERA_SUPERVISOR_PERIOD, registry);

    std::function<void(int, std::string)> callback = std::bind(&NMEA_controller::parse, nmea_controller, std::placeholders::_1, std::placeholders::_2);
    uart_1->register_pattern_detected_callback(callback);

    registry->internal_set<string>("control.mode", "off");
    registry->internal_set<float>("control.motors.front_left", 0.0);
    registry->internal_set<float>("control.motors.front_right", 0.0);
    registry->internal_set<float>("control.motors.rear_left", 0.0);
    registry->internal_set<float>("control.motors.rear_right", 0.0);

#endif

    Task::delay_ms(1500);

#ifdef DATA_RECORDING
    data_recorder->start();
#else
    wifi->connect();
    rate_controller->start();
    attitude_controller->start();
    height_controller->start();
#endif

    while (true)
    {
        udp->send_broadcast("{\"announcement\":\"kwadcopter\"}", 5001);
        Task::delay_ms(1000);
    }
}
