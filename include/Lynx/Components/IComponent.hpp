#pragma once

using uint32_t = unsigned int;

#include <memory>

namespace Cutlass
{
    class Context;
}

namespace Lynx
{
    class IComponent
    {
    public:
        IComponent()
        : mUpdateFlag(true)
        //, mRequireDestroyFlag(false)
        {
            static uint32_t IDGen = 0;
            mID = IDGen++;
        }

        virtual ~IComponent(){};

        virtual void update() = 0;
        
        //どうしても識別したいときに使う        
        uint32_t getID() const
        {
            return mID;
        }

        //更新が不要な場合はセットする
        //普通は呼ばなくていい
        void setUpdateFlag(const bool flag)
        {
            mUpdateFlag = flag;
        }

        const bool getUpdateFlag() const
        {
            return mUpdateFlag;
        }

        // void requireDestroy()
        // {
        //     mRequireDestroyFlag = true;
        // }

        // bool isDestroyRequired() const
        // {
        //     return mRequireDestroyFlag;
        // }
        
    private:
        uint32_t mID;
        bool mUpdateFlag;
        //bool mRequireDestroyFlag;
    };
}
