#pragma once

#include "IComponent.hpp"

#include <portaudio.h>

#include <vector>
#include <chrono>
#include <optional>

namespace Lynx
{
    class SoundComponent : public IComponent
    {
    public:

        struct WaveFormat
        {
            unsigned short format_id;   //フォーマットID
            unsigned short channel; //チャンネル数 monaural=1 , stereo=2
            unsigned long  sampling_rate;//１秒間のサンプル数，サンプリングレート(Hz),だいたいは44.1kHz
            unsigned long  bytes_per_sec;//１秒間のデータサイズ   44.1kHz 16bit ステレオ ならば44100×2×2 = 176400
            unsigned short block_size;  //１ブロックのサイズ．8bit:nomaural=1byte , 16bit:stereo=4byte
            unsigned short bits_per_sample;//１サンプルのビット数 8bit or 16bit
        };

        SoundComponent();

        virtual ~SoundComponent();

        void create(const char* pathToWavFile, bool loop = false);

        template<typename T> T readData()
        {
            //PCMデータを読んで波形を返す
            if(mLoopFlag && getEndFlag())
            {
                //ループフラグが立ってる場合、巻き戻し
                mBufPos = 0;
            }
            if(mFormat.bits_per_sample == 8)
            {
                //8bit
                return data_8bit[mBufPos++];
            }
            else if(mFormat.bits_per_sample == 16)
            {
                //16bit
                return data_16bit[mBufPos++];
            }

            return 0;
        }

        void setVolumeRate(float volumeRate);
        float getVolumeRate() const;

        const WaveFormat& getWaveFormat() const; 

        bool getEndFlag();

        void setLoop(bool flag);
        bool getLoopFlag() const;

        double getPlayingTime() const;

        //PaStream* getpStream();

        void play();
        void stop();

        virtual void update();

    private:

        static uint32_t instanceCount;

        bool mLoaded;

        std::optional<PaStream*> mpStream;
        float mVolumeRate;

        WaveFormat mFormat;                    //フォーマット情報
        std::vector<unsigned char> data_8bit;   //音データ(8bitの場合)
        std::vector<short> data_16bit;          //音データ(16bitの場合)

        int mRIFFFileSize;                   //RIFFヘッダから読んだファイルサイズ
        int mPCMDataSize;                    //実際のデータサイズ
        int mBufPos;                         //バッファポジション
        bool mLoopFlag;                      //ループフラグ
        bool mPlayFlag;                      //再生中かどうか
        std::chrono::high_resolution_clock::time_point mStartTime;
        double mPlayingDuration;
    };
}