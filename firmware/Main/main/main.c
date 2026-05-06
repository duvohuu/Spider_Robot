#include "pca9685.h"
#include "gait.h"

void app_main(void)
{
    i2c_master_init();
    pca_init(PCA_L);
    pca_init(PCA_R);

    calibration();
    tripod_init();

    while (1) {
        tripod_step();
        // test_tibia_mechanical();
        // test_femur();

    }
}