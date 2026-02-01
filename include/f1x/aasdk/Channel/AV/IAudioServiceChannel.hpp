#pragma once

#include <memory>
#include <aasdk_proto/AVChannelSetupResponseMessage.pb.h>
#include <aasdk_proto/AVMediaAckIndicationMessage.pb.h>
#include <aasdk_proto/ChannelOpenResponseMessage.pb.h>
#include <f1x/aasdk/Messenger/ChannelId.hpp>
#include <f1x/aasdk/Channel/Promise.hpp>
#include <f1x/aasdk/Channel/AV/IAudioContextChannelEventHandler.hpp>

namespace f1x
{
    namespace aasdk
    {
        namespace channel
        {
            namespace av
            {

                class IAudioContextChannel
                {
                public:
                    using Pointer = std::shared_ptr<IAudioContextChannel>;

                    IAudioContextChannel() = default;
                    virtual ~IAudioContextChannel() = default;

                    virtual void receive(IAudioContextChannelEventHandler::Pointer eventHandler) = 0;
                    virtual void sendChannelOpenResponse(const proto::messages::ChannelOpenResponse& response,
                        SendPromise::Pointer promise) = 0;
                    virtual void sendAVChannelSetupResponse(const proto::messages::AVChannelSetupResponse& response,
                        SendPromise::Pointer promise) = 0;
                    virtual void sendAVMediaAckIndication(const proto::messages::AVMediaAckIndication& indication,
                        SendPromise::Pointer promise) = 0;
                    virtual messenger::ChannelId getId() const = 0;
                };

            } // namespace av
        } // namespace channel
    } // namespace aasdk
} // namespace f1x
