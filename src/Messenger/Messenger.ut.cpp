/*
*  This file is part of aasdk library project.
*  Copyright (C) 2018 f1x.studio (Michal Szwaj)
*
*  aasdk is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 3 of the License, or
*  (at your option) any later version.

*  aasdk is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with aasdk. If not, see <http://www.gnu.org/licenses/>.
*/

#include <boost/test/unit_test.hpp>
#include <f1x/aasdk/Messenger/UT/MessageInStream.mock.hpp>
#include <f1x/aasdk/Messenger/UT/MessageOutStream.mock.hpp>
#include <f1x/aasdk/Messenger/UT/ReceivePromiseHandler.mock.hpp>
#include <f1x/aasdk/Messenger/UT/SendPromiseHandler.mock.hpp>
#include <f1x/aasdk/Messenger/Messenger.hpp>

namespace f1x
{
namespace aasdk
{
namespace messenger
{
namespace ut
{

using ::testing::_;
using ::testing::SaveArg;
using ::testing::Return;

class MessengerUnitTest
{
protected:
    MessengerUnitTest()
        : messageInStream_(&messageInStreamMock_, [](auto*) {})
        , messageOutStream_(&messageOutStreamMock_, [](auto*) {})
        , receivePromise_(ReceivePromise::defer(ioContext_))
        , sendPromise_(SendPromise::defer(ioContext_))
    {
        receivePromise_->then(std::bind(&ReceivePromiseHandlerMock::onResolve, &receivePromiseHandlerMock_, std::placeholders::_1),
                             std::bind(&ReceivePromiseHandlerMock::onReject, &receivePromiseHandlerMock_, std::placeholders::_1));

        sendPromise_->then(std::bind(&SendPromiseHandlerMock::onResolve, &sendPromiseHandlerMock_),
                          std::bind(&SendPromiseHandlerMock::onReject, &sendPromiseHandlerMock_, std::placeholders::_1));
    }

    boost::asio::io_context ioContext_;
    MessageInStreamMock messageInStreamMock_;
    IMessageInStream::Pointer messageInStream_;
    MessageOutStreamMock messageOutStreamMock_;
    IMessageOutStream::Pointer messageOutStream_;
    ReceivePromiseHandlerMock receivePromiseHandlerMock_;
    ReceivePromise::Pointer receivePromise_;
    SendPromiseHandlerMock sendPromiseHandlerMock_;
    SendPromise::Pointer sendPromise_;
};

BOOST_FIXTURE_TEST_CASE(Messenger_Receive, MessengerUnitTest)
{
    Messenger::Pointer themessenger(std::make_shared<Messenger>(ioContext_, messageInStream_, messageOutStream_));
    themessenger->enqueueReceive(ChannelId::MEDIA_AUDIO, std::move(receivePromise_));

    ReceivePromise::Pointer inStreamReceivePromise;
    EXPECT_CALL(messageInStreamMock_, startReceive(_)).WillOnce(SaveArg<0>(&inStreamReceivePromise));

    ioContext_.run();
    ioContext_.reset();

    Message::Pointer message(std::make_shared<Message>(ChannelId::MEDIA_AUDIO, EncryptionType::ENCRYPTED, MessageType::SPECIFIC));
    inStreamReceivePromise->resolve(message);

    EXPECT_CALL(receivePromiseHandlerMock_, onReject(_)).Times(0);
    EXPECT_CALL(receivePromiseHandlerMock_, onResolve(message));
    ioContext_.run();
}

BOOST_FIXTURE_TEST_CASE(Messenger_DirectReceive, MessengerUnitTest)
{
    Messenger::Pointer themessenger(std::make_shared<Messenger>(ioContext_, messageInStream_, messageOutStream_));
    themessenger->enqueueReceive(ChannelId::MEDIA_AUDIO, std::move(receivePromise_));

    ReceivePromise::Pointer inStreamReceivePromise;
    EXPECT_CALL(messageInStreamMock_, startReceive(_)).WillRepeatedly(SaveArg<0>(&inStreamReceivePromise));

    ioContext_.run();
    ioContext_.reset();

    Message::Pointer inputChannelMessage(std::make_shared<Message>(ChannelId::INPUT, EncryptionType::ENCRYPTED, MessageType::SPECIFIC));
    inStreamReceivePromise->resolve(inputChannelMessage);

    ioContext_.run();
    ioContext_.reset();

    auto secondReceivePromise = ReceivePromise::defer(ioContext_);
    secondReceivePromise->then(std::bind(&ReceivePromiseHandlerMock::onResolve, &receivePromiseHandlerMock_, std::placeholders::_1),
                              std::bind(&ReceivePromiseHandlerMock::onReject, &receivePromiseHandlerMock_, std::placeholders::_1));
    themessenger->enqueueReceive(ChannelId::INPUT, std::move(secondReceivePromise));

    EXPECT_CALL(receivePromiseHandlerMock_, onReject(_)).Times(0);
    EXPECT_CALL(receivePromiseHandlerMock_, onResolve(inputChannelMessage));

    ioContext_.run();
    ioContext_.reset();

    Message::Pointer audioChannelMessage(std::make_shared<Message>(ChannelId::MEDIA_AUDIO, EncryptionType::ENCRYPTED, MessageType::SPECIFIC));
    inStreamReceivePromise->resolve(audioChannelMessage);

    EXPECT_CALL(receivePromiseHandlerMock_, onReject(_)).Times(0);
    EXPECT_CALL(receivePromiseHandlerMock_, onResolve(audioChannelMessage));
    ioContext_.run();
}

BOOST_FIXTURE_TEST_CASE(Messenger_OnlyOneReceiveAtATime, MessengerUnitTest)
{
    Messenger::Pointer themessenger(std::make_shared<Messenger>(ioContext_, messageInStream_, messageOutStream_));
    themessenger->enqueueReceive(ChannelId::MEDIA_AUDIO, std::move(receivePromise_));

    ReceivePromise::Pointer inStreamReceivePromise;
    EXPECT_CALL(messageInStreamMock_, startReceive(_)).WillOnce(SaveArg<0>(&inStreamReceivePromise));

    ioContext_.run();
    ioContext_.reset();

    auto secondReceivePromise = ReceivePromise::defer(ioContext_);
    secondReceivePromise->then(std::bind(&ReceivePromiseHandlerMock::onResolve, &receivePromiseHandlerMock_, std::placeholders::_1),
                              std::bind(&ReceivePromiseHandlerMock::onReject, &receivePromiseHandlerMock_, std::placeholders::_1));
    themessenger->enqueueReceive(ChannelId::INPUT, std::move(secondReceivePromise));

    ioContext_.run();
    ioContext_.reset();

    const error::Error e(error::ErrorCode::USB_TRANSFER, 41);
    inStreamReceivePromise->reject(e);

    EXPECT_CALL(receivePromiseHandlerMock_, onReject(e)).Times(2);
    EXPECT_CALL(receivePromiseHandlerMock_, onResolve(_)).Times(0);

    ioContext_.run();
}

BOOST_FIXTURE_TEST_CASE(Messenger_Send, MessengerUnitTest)
{
    Messenger::Pointer themessenger(std::make_shared<Messenger>(ioContext_, messageInStream_, messageOutStream_));

    Message::Pointer message(std::make_shared<Message>(ChannelId::MEDIA_AUDIO, EncryptionType::ENCRYPTED, MessageType::SPECIFIC));
    themessenger->enqueueSend(message, std::move(sendPromise_));

    SendPromise::Pointer outStreamSendPromise;
    EXPECT_CALL(messageOutStreamMock_, stream(message, _)).WillOnce(SaveArg<1>(&outStreamSendPromise));

    ioContext_.run();
    ioContext_.reset();

    EXPECT_CALL(sendPromiseHandlerMock_, onReject(_)).Times(0);
    EXPECT_CALL(sendPromiseHandlerMock_, onResolve());

    outStreamSendPromise->resolve();
    ioContext_.run();
}

BOOST_FIXTURE_TEST_CASE(Messenger_OnlyOneSendAtATime, MessengerUnitTest)
{
    Messenger::Pointer themessenger(std::make_shared<Messenger>(ioContext_, messageInStream_, messageOutStream_));

    Message::Pointer message(std::make_shared<Message>(ChannelId::MEDIA_AUDIO, EncryptionType::ENCRYPTED, MessageType::SPECIFIC));
    themessenger->enqueueSend(message, std::move(sendPromise_));

    SendPromise::Pointer outStreamSendPromise;
    EXPECT_CALL(messageOutStreamMock_, stream(message, _)).Times(2).WillRepeatedly(SaveArg<1>(&outStreamSendPromise));

    ioContext_.run();
    ioContext_.reset();

    auto secondSendPromise = SendPromise::defer(ioContext_);
    secondSendPromise->then(std::bind(&SendPromiseHandlerMock::onResolve, &sendPromiseHandlerMock_),
                           std::bind(&SendPromiseHandlerMock::onReject, &sendPromiseHandlerMock_, std::placeholders::_1));
    themessenger->enqueueSend(message, std::move(secondSendPromise));

    ioContext_.run();
    ioContext_.reset();

    EXPECT_CALL(sendPromiseHandlerMock_, onReject(_)).Times(0);
    EXPECT_CALL(sendPromiseHandlerMock_, onResolve()).Times(2);
    outStreamSendPromise->resolve();
    ioContext_.run();
    ioContext_.reset();

    outStreamSendPromise->resolve();
    ioContext_.run();
}

BOOST_FIXTURE_TEST_CASE(Messenger_SendFailed, MessengerUnitTest)
{
    Messenger::Pointer themessenger(std::make_shared<Messenger>(ioContext_, messageInStream_, messageOutStream_));

    Message::Pointer message(std::make_shared<Message>(ChannelId::MEDIA_AUDIO, EncryptionType::ENCRYPTED, MessageType::SPECIFIC));
    themessenger->enqueueSend(message, std::move(sendPromise_));

    SendPromise::Pointer outStreamSendPromise;
    EXPECT_CALL(messageOutStreamMock_, stream(message, _)).WillOnce(SaveArg<1>(&outStreamSendPromise));

    ioContext_.run();
    ioContext_.reset();

    auto secondSendPromise = SendPromise::defer(ioContext_);
    secondSendPromise->then(std::bind(&SendPromiseHandlerMock::onResolve, &sendPromiseHandlerMock_),
                           std::bind(&SendPromiseHandlerMock::onReject, &sendPromiseHandlerMock_, std::placeholders::_1));
    themessenger->enqueueSend(message, std::move(secondSendPromise));

    ioContext_.run();
    ioContext_.reset();

    error::Error e(error::ErrorCode::USB_TRANSFER, 67);
    outStreamSendPromise->reject(e);

    EXPECT_CALL(sendPromiseHandlerMock_, onReject(e)).Times(2);
    EXPECT_CALL(sendPromiseHandlerMock_, onResolve()).Times(0);
    ioContext_.run();
}

}
}
}
}
