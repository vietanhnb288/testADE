/**
 * Copyright (c) 2015 - 2020, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include "nrf_drv_spi.h"
#include "app_util_platform.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "boards.h"
#include "app_error.h"
#include <string.h>
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

//#include "ade_lib.h"

#include "ade9153a.h"
#include "nrf5_ade9153a.h"

#define reset_pin 4
#define ctrl_pin 18
//#define SPI_INSTANCE  0 /**< SPI instance index. */
//static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(SPI_INSTANCE);  /**< SPI instance. */
//static volatile bool spi_xfer_done;  /**< Flag used to indicate that SPI instance completed the transfer. */

//#define TEST_STRING "Nordic"
//static uint8_t       m_tx_buf[] = TEST_STRING;           /**< TX buffer. */
//static uint8_t       m_rx_buf[sizeof(TEST_STRING) + 1];    /**< RX buffer. */
//static const uint8_t m_length = sizeof(m_tx_buf);        /**< Transfer length. */
//uint16_t cmd_hdr;
/**
 * @brief SPI user event handler.
 * @param event
 */
//void spi_event_handler(nrf_drv_spi_evt_t const * p_event,
//                       void *                    p_context)
//{
//    spi_xfer_done = true;
//    NRF_LOG_INFO("Transfer completed.");
//    if (m_rx_buf[0] != 0)
//    {
//        NRF_LOG_INFO(" Received:");
//        NRF_LOG_HEXDUMP_INFO(m_rx_buf, strlen((const char *)m_rx_buf));
//    }
//}
// *                        DEFINE
// */
#define RSHUNT 0.001 // Ohm
#define PGAGAIN 16
#define RBIG 1000000 // Ohm
#define RSMALL 1000  // Ohm

/*****************************************************************
 *                        VARIABLE
 */
RMSRegs_t pRMSRegs;
PowRegs_t pPowRegs;
PQRegs_t pPQRegs;
ACALRegs_t pACALRegs;

float pVHeadRoom;
uint8_t ADE_acal = 0;
bool checkADE = false;
int main(void)
{		
    bsp_board_init(BSP_INIT_LEDS);

    APP_ERROR_CHECK(NRF_LOG_INIT(NULL));
    NRF_LOG_DEFAULT_BACKENDS_INIT();
	
		NRF_LOG_INFO("Template example started.");
	
		pRMSRegs.targetAICC = calculate_target_aicc(RSHUNT, PGAGAIN);
    NRF_LOG_INFO("targetAICC: " NRF_LOG_FLOAT_MARKER, NRF_LOG_FLOAT(pRMSRegs.targetAICC));
    pRMSRegs.targetAVCC = calculate_target_avcc(RBIG, RSMALL, &pVHeadRoom);
    NRF_LOG_INFO("targetAVCC: " NRF_LOG_FLOAT_MARKER, NRF_LOG_FLOAT(pRMSRegs.targetAVCC));
    pPowRegs.targetPowCC = calculate_target_powCC(pRMSRegs.targetAICC, pRMSRegs.targetAVCC);
    NRF_LOG_INFO("targetPowCC: " NRF_LOG_FLOAT_MARKER, NRF_LOG_FLOAT(pPowRegs.targetPowCC));

    NRF_LOG_FLUSH();
		nrf_gpio_pin_set(ctrl_pin);
		ADE9153_reset(reset_pin);
	
		init_spiADE9153();
	
  
	
		uint32_t vPrt = spi_read32(REG_VERSION_PRODUCT);
    NRF_LOG_INFO("VERSION PRODUCT: %x", vPrt);
	
		ADE9153_initCFG();
	
		spi_write32((REG_AIGAIN), -268435456);


    /* Autocalibration ADE9153 */
    ADE_acal = 10; // 500ms/times
    NRF_LOG_INFO("Autocalibration AI \n");
    checkADE = ADE9153_acal_AINormal();
    while (ADE_acal && !checkADE)
    {
        checkADE = ADE9153_acal_AINormal();
        if (!checkADE)
        {
            ADE_acal--;
            nrf_delay_ms(500);
            NRF_LOG_INFO("AutoCalb_AI: nADE_acal %d\n", ADE_acal);
        }
        else
        {
            NRF_LOG_INFO("checkADE_ok - nADE_acal: %d\n", ADE_acal);
        }
    }
    if (!checkADE)
    {
        NRF_LOG_INFO("checkADE_false\n");
    }
    else
    {
        nrf_delay_ms(20 * 1000);
        NRF_LOG_INFO("Autocalibration stop AI\n");
        ADE9153_acal_stop();
    }
    NRF_LOG_FLUSH();

    ADE_acal = 10; // 500ms/times
    NRF_LOG_INFO("Autocalibration AV\n");
    checkADE = ADE9153_acal_AV();
    while (ADE_acal && !checkADE)
    {
        checkADE = ADE9153_acal_AV();
        if (!checkADE)
        {
            ADE_acal--;
            nrf_delay_ms(500);
        }
        else
        {
            NRF_LOG_INFO("checkADE_ok - nADE_acal: %d\n", ADE_acal);
        }
    }
    if (!checkADE)
    {
        NRF_LOG_INFO("checkADE_false");
    }
    else
    {
        nrf_delay_ms(40 * 1000);
        NRF_LOG_INFO("Autocalibration stop AV");
        ADE9153_acal_stop();
    }
    nrf_delay_ms(500);
    NRF_LOG_FLUSH();

    /* Read mSure autocalibration */
    ADE9153_acal_result(&pACALRegs);
		
		uint32_t a = spi_read32(REG_AIGAIN);
		NRF_LOG_INFO("Test doc gia tri thanh ghi REG_AIGAIN : %x",a);

    /* Config AIGAIN & AVGAIN */
    ADE9153_AIGainCFG(pRMSRegs.targetAICC, pACALRegs.mSureAICCValue);
    ADE9153_AVGainCFG(pRMSRegs.targetAVCC, pACALRegs.mSureAVCCValue);
		while (1)
		{
		 
		ADE9153_read_RMSRegs(&pRMSRegs);
    ADE9153_read_PowRegs(&pPowRegs);
    ADE9153_read_PQRegs(&pPQRegs);
						NRF_LOG_INFO("RMSRegs:");
            NRF_LOG_INFO("AIReg: %x and AIValue: " NRF_LOG_FLOAT_MARKER, pRMSRegs.AIReg, NRF_LOG_FLOAT(pRMSRegs.AIValue));
            NRF_LOG_INFO("AVReg: %x and AVValue: " NRF_LOG_FLOAT_MARKER, pRMSRegs.AVReg, NRF_LOG_FLOAT(pRMSRegs.AVValue));
            NRF_LOG_INFO("PowRegs:");
            NRF_LOG_INFO("activeReg: %x and activeValue: " NRF_LOG_FLOAT_MARKER, pPowRegs.activeReg, NRF_LOG_FLOAT(pPowRegs.activeValue));
            NRF_LOG_INFO("reactiveReg: %x and reactiveValue: " NRF_LOG_FLOAT_MARKER, pPowRegs.reactiveReg, NRF_LOG_FLOAT(pPowRegs.reactiveValue));
            NRF_LOG_INFO("apparentReg: %x and apparentValue: " NRF_LOG_FLOAT_MARKER, pPowRegs.apparentReg, NRF_LOG_FLOAT(pPowRegs.apparentValue));
            NRF_LOG_INFO("PF compute: " NRF_LOG_FLOAT_MARKER, NRF_LOG_FLOAT(pPowRegs.activeValue / pPowRegs.apparentValue));
            NRF_LOG_INFO("PQRegs:");
            NRF_LOG_INFO("pwFactorReg: %x and pwFactorValue: " NRF_LOG_FLOAT_MARKER, pPQRegs.pwFactorReg, NRF_LOG_FLOAT(pPQRegs.pwFactorValue));
            NRF_LOG_INFO("periodReg: %x and freqValue: " NRF_LOG_FLOAT_MARKER, pPQRegs.periodReg, NRF_LOG_FLOAT(pPQRegs.freqValue));
            NRF_LOG_INFO("angleAV_AIReg: %x and angleAV_AIValue: " NRF_LOG_FLOAT_MARKER "\n", pPQRegs.angleAV_AIReg, NRF_LOG_FLOAT(pPQRegs.angleAV_AIValue));

    NRF_LOG_FLUSH();
		bsp_board_led_invert(BSP_BOARD_LED_0);
    nrf_delay_ms(5000);
			
		}
   
}
