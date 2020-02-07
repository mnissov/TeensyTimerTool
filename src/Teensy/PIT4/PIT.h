#pragma once

#include "PITChannel.h"

namespace TeensyTimerTool
{
    class PIT_t
    {
     public:
        inline static ITimerChannel* getTimer();

     protected:
        static bool isInitialized;
        static void isr();
        static callback_t callback;
        static PITChannel* channel;
        static callback_t callbacks[4];

        static constexpr IMXRT_PIT_CHANNEL_t* channels = IMXRT_PIT_CHANNELS;
        static constexpr IMXRT_PIT_CHANNEL_t* tmp = &IMXRT_PIT_CHANNELS[0];

        static constexpr auto x = &tmp -> CVAL;

        // the following is calculated at compile time
        //static constexpr IRQ_NUMBER_t irq = moduleNr == 0 ? IRQ_GPT1 : IRQ_GPT2;
        //static IMXRT_GPT_t* const pGPT;
        //static_assert(moduleNr < 2, "Wrong GPT Number");
    };

    // IMPLEMENTATION ===========================================================================

    //template <unsigned moduleNr>
    //IMXRT_GPT_t* const GPT_t<moduleNr>::pGPT = reinterpret_cast<IMXRT_GPT_t*>(moduleNr == 0 ? &IMXRT_GPT1 : &IMXRT_GPT2);

    ITimerChannel* PIT_t::getTimer()
    {
        if (!isInitialized)
        {
            isInitialized = true;

            CCM_CCGR1 |= CCM_CCGR1_PIT(CCM_CCGR_ON);
            PIT_MCR = 1;

            if (USE_GPT_PIT_150MHz)                       // timer clock setting from config.h
                CCM_CSCMR1 &= ~CCM_CSCMR1_PERCLK_CLK_SEL; // 150MHz
            else
                CCM_CSCMR1 |= CCM_CSCMR1_PERCLK_CLK_SEL;  // 24MHz

            for (int i = 0; i < 4; i++)
            {
                IMXRT_PIT_CHANNELS[i].TCTRL = 0u;
            }

            IMXRT_PIT_CHANNELS[2].LDVAL = 100;
            IMXRT_PIT_CHANNELS[2].TCTRL = PIT_TCTRL_TIE | PIT_TCTRL_TEN ;
            callbacks[2] = [] {
                digitalWriteFast(0, HIGH);
                delayNanoseconds(200);
                digitalWriteFast(0, LOW);
            };

            attachInterruptVector(IRQ_PIT, isr);
            NVIC_ENABLE_IRQ(IRQ_PIT);
        }



        return nullptr;
    }

    inline void PIT_t::isr()
    {
        if (callbacks[0] != nullptr && IMXRT_PIT_CHANNELS[0].TFLG)
        {
            IMXRT_PIT_CHANNELS[0].TFLG = 1;
            callbacks[0]();           
        }
        if (callbacks[1] != nullptr && IMXRT_PIT_CHANNELS[1].TFLG)
        {
            IMXRT_PIT_CHANNELS[1].TFLG = 1;
            callbacks[1]();
        }
        if (callbacks[2] != nullptr && IMXRT_PIT_CHANNELS[2].TFLG)
        {
            IMXRT_PIT_CHANNELS[2].TFLG = 1;
            callbacks[2]();
        }
        if (callbacks[3] != nullptr && IMXRT_PIT_CHANNELS[3].TFLG)
        {
            IMXRT_PIT_CHANNELS[3].TFLG = 1;
            callbacks[3]();
        }       
        
        asm volatile("dsb"); //wait until register changes propagated through the cache
        
        // if (!channel->isPeriodic)
        //     pGPT->CR &= ~GPT_CR_EN; // stop timer in one shot mode

        // pGPT->SR = 0x3F; // reset all interrupt flags
        // callback();      // we only enabled the OF1 interrupt-> no need to find out which interrupt was actually called
        //                  //pGPT->SR;                 // re-read flag to prevent re entering, takes very long, seems to work without?
    }
}
