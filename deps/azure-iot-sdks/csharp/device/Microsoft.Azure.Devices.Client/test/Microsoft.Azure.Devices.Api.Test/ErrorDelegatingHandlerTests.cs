﻿// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
namespace Microsoft.Azure.Devices.Client.Test
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Net.Sockets;
    using System.Threading.Tasks;
    using Microsoft.Azure.Devices.Client.Common;
    using Microsoft.Azure.Devices.Client.Exceptions;
    using Microsoft.Azure.Devices.Client.Transport;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using NSubstitute;

    [TestClass]
    public class ErrorDelegatingHandlerTests
    {

        internal static readonly HashSet<Type> NonTransientExceptions = new HashSet<Type>
        {
            typeof(MessageTooLargeException),
            typeof(DeviceMessageLockLostException),
            typeof(UnauthorizedException),
            typeof(IotHubNotFoundException),
            typeof(DeviceNotFoundException),
            typeof(QuotaExceededException),
            typeof(IotHubException),
        };

        const string ErrorMessage = "Error occured.";

        static readonly Dictionary<Type, Func<Exception>> ExceptionFactory = new Dictionary<Type, Func<Exception>>
        {
            { typeof(UnauthorizedException), () => new UnauthorizedException(ErrorMessage) },
            { typeof(IotHubNotFoundException), () => new IotHubNotFoundException(ErrorMessage) },
            { typeof(DeviceNotFoundException), () => new DeviceNotFoundException(ErrorMessage) },
            { typeof(QuotaExceededException), () => new QuotaExceededException(ErrorMessage) },
            { typeof(IotHubClientTransientException), () => new IotHubClientTransientException(ErrorMessage) },
            { typeof(IotHubCommunicationException), () => new IotHubCommunicationException(ErrorMessage) },
            { typeof(MessageTooLargeException), () => new MessageTooLargeException(ErrorMessage) },
            { typeof(DeviceMessageLockLostException), () => new DeviceMessageLockLostException(ErrorMessage) },
            { typeof(ServerBusyException), () => new ServerBusyException(ErrorMessage) },
            { typeof(IotHubException), () => new IotHubException(ErrorMessage) },
            { typeof(IOException), () => new IOException(ErrorMessage) },
            { typeof(TimeoutException), () => new TimeoutException(ErrorMessage) },
            { typeof(ObjectDisposedException), () => new ObjectDisposedException(ErrorMessage) },
            { typeof(OperationCanceledException), () => new OperationCanceledException(ErrorMessage) },
            { typeof(TaskCanceledException), () => new TaskCanceledException(ErrorMessage) },
            { typeof(SocketException), () => new SocketException(1) },
        };
            
        [TestMethod]
        [TestCategory("CIT")]
        [TestCategory("DelegatingHandlers")]
        [TestCategory("Owner [mtuchkov]")]
        public async Task ErrorHandler_NoErrors_Success()
        {
            var innerHandler = Substitute.For<IDelegatingHandler>();
            innerHandler.OpenAsync(Arg.Is(false)).Returns(TaskConstants.Completed);
            innerHandler.SendEventAsync(Arg.Any<Message>()).Returns(TaskConstants.Completed);
            var sut = new ErrorDelegatingHandler(() => innerHandler);

            //emulate Gatekeeper behaviour: it opens the channel for us
            await sut.OpenAsync(false);
            await sut.SendEventAsync(new Message(new byte[0]));

            await innerHandler.Received(1).OpenAsync(Arg.Is(false));
            await innerHandler.Received(1).SendEventAsync(Arg.Any<Message>());
        }

        [TestMethod]
        [TestCategory("CIT")]
        [TestCategory("DelegatingHandlers")]
        [TestCategory("Owner [mtuchkov]")]
        public async Task ErrorHandler_TransientErrorOccuredChannelIsAlive_ChannelIsTheSame()
        {
            foreach (Type exceptionType in ErrorDelegatingHandler.TransportTransientExceptions)
            {
                await TestExceptionThrown(exceptionType, typeof(IotHubClientTransientException), false);
            }
        }

        [TestMethod]
        [TestCategory("CIT")]
        [TestCategory("DelegatingHandlers")]
        [TestCategory("Owner [mtuchkov]")]
        public async Task ErrorHandler_TransientErrorOccured_ChannelIsRecreated()
        {
            foreach (Type exceptionType in ErrorDelegatingHandler.TransientExceptions)
            {
                if (!ErrorDelegatingHandler.TransportTransientExceptions.Contains(exceptionType))
                {
                    await TestExceptionThrown(exceptionType, typeof(IotHubClientTransientException), true);
                }
            }
        }

        [TestMethod]
        [TestCategory("CIT")]
        [TestCategory("DelegatingHandlers")]
        [TestCategory("Owner [mtuchkov]")]
        public async Task ErrorHandler_NonTransientErrorOccured_ChannelIsRecreated()
        {
            foreach (Type exceptionType in NonTransientExceptions)
            {
                await TestExceptionThrown(exceptionType, exceptionType, true);
            }
        }

        static async Task TestExceptionThrown(Type thrownExceptionType, Type expectedExceptionType, bool reopenExpected)
        {
            var message = new Message(new byte[0]);

            await OperationAsync_ExceptionThrownAndThenSucceed_OperationSuccessfullyCompleted(
                di => di.SendEventAsync(Arg.Is(message)), 
                di => di.SendEventAsync(message), 
                di => di.Received(2).SendEventAsync(Arg.Is(message)), 
                thrownExceptionType, expectedExceptionType, reopenExpected);

            IEnumerable<Message> messages = new[] { new Message(new byte[0])};

            await OperationAsync_ExceptionThrownAndThenSucceed_OperationSuccessfullyCompleted(
                di => di.SendEventAsync(Arg.Is(messages)), 
                di => di.SendEventAsync(messages), 
                di => di.Received(2).SendEventAsync(Arg.Is(messages)), 
                thrownExceptionType, expectedExceptionType, reopenExpected);

            await OpenAsync_ExceptionThrownAndThenSucceed_SuccessfullyOpened(
                di => di.OpenAsync(Arg.Is(false)),
                di => di.OpenAsync(false),
                di => di.Received(2).OpenAsync(Arg.Is(false)),
                thrownExceptionType, expectedExceptionType);

            string lockToken = "lockToken";

            await OperationAsync_ExceptionThrownAndThenSucceed_OperationSuccessfullyCompleted(
                di => di.CompleteAsync(Arg.Is(lockToken)),
                di => di.CompleteAsync(lockToken),
                di => di.Received(2).CompleteAsync(Arg.Is(lockToken)),
                thrownExceptionType, expectedExceptionType, reopenExpected);

            await OperationAsync_ExceptionThrownAndThenSucceed_OperationSuccessfullyCompleted(
                di => di.AbandonAsync(Arg.Is(lockToken)),
                di => di.AbandonAsync(lockToken),
                di => di.Received(2).AbandonAsync(Arg.Is(lockToken)),
                thrownExceptionType, expectedExceptionType, reopenExpected);

            await OperationAsync_ExceptionThrownAndThenSucceed_OperationSuccessfullyCompleted(
                di => di.RejectAsync(Arg.Is(lockToken)),
                di => di.RejectAsync(lockToken),
                di => di.Received(2).RejectAsync(Arg.Is(lockToken)),
                thrownExceptionType, expectedExceptionType, reopenExpected);

            TimeSpan timeout = TimeSpan.FromSeconds(1);
            await OperationAsync_ExceptionThrownAndThenSucceed_OperationSuccessfullyCompleted(
                di => di.ReceiveAsync(Arg.Is(timeout)),
                di => di.ReceiveAsync(timeout),
                di => di.Received(2).ReceiveAsync(Arg.Is(timeout)),
                thrownExceptionType, expectedExceptionType, reopenExpected);

            await OperationAsync_ExceptionThrownAndThenSucceed_OperationSuccessfullyCompleted(
                di => di.ReceiveAsync(),
                di => di.ReceiveAsync(),
                di => di.Received(2).ReceiveAsync(),
                thrownExceptionType, expectedExceptionType, reopenExpected);
        }

        static async Task OperationAsync_ExceptionThrownAndThenSucceed_OperationSuccessfullyCompleted(Func<IDelegatingHandler, Task<Message>> mockSetup, Func<IDelegatingHandler, Task<Message>> act, Func<IDelegatingHandler, Task<Message>> assert, Type thrownExceptionType, Type expectedExceptionType, bool reopenExpected)
        {
            int ctorCallCounter = 0;
            var innerHandler = Substitute.For<IDelegatingHandler>();
            var sut = new ErrorDelegatingHandler(() =>
            {
                ctorCallCounter++;
                return innerHandler;
            });

            //initial OpenAsync to emulate Gatekeeper behaviour
            innerHandler.OpenAsync(Arg.Is(false)).Returns(TaskConstants.Completed);
            await sut.OpenAsync(false);

            //set initial operation result that throws

            bool[] setup = { false };
            mockSetup(innerHandler).Returns(ci =>
            {
                if (setup[0])
                {
                    return Task.FromResult(new Message());
                }
                throw ExceptionFactory[thrownExceptionType]();
            });

            //act
            await ((Func<Task>)(() => act(sut))).ExpectedAsync(expectedExceptionType);

            //override outcome
            setup[0] = true;//otherwise previosly setup call will happen and throw;
            mockSetup(innerHandler).Returns(new Message());

            //act
            await act(sut);

            //assert
            await innerHandler.Received(reopenExpected ? 2 : 1).OpenAsync(Arg.Is(false));
            await assert(innerHandler);
            Assert.AreEqual(reopenExpected ? 2 : 1, ctorCallCounter);
        }

        static async Task OperationAsync_ExceptionThrownAndThenSucceed_OperationSuccessfullyCompleted(Func<IDelegatingHandler, Task> mockSetup, Func<IDelegatingHandler, Task> act, Func<IDelegatingHandler, Task> assert, Type thrownExceptionType, Type expectedExceptionType, bool reopenExpected)
        {
            int ctorCallCounter = 0;
            var innerHandler = Substitute.For<IDelegatingHandler>();
            var sut = new ErrorDelegatingHandler(() =>
            {
                ctorCallCounter++;
                return innerHandler;
            });

            //initial OpenAsync to emulate Gatekeeper behaviour
            innerHandler.OpenAsync(Arg.Is(false)).Returns(TaskConstants.Completed);
            await sut.OpenAsync(false);

            //set initial operation result that throws

            bool[] setup = { false };
            mockSetup(innerHandler).Returns(ci =>
            {
                if (setup[0])
                {
                    return TaskConstants.Completed;
                }
                throw ExceptionFactory[thrownExceptionType]();
            });

            //act
            await ((Func<Task>)(() => act(sut))).ExpectedAsync(expectedExceptionType);

            //override outcome
            setup[0] = true;//otherwise previosly setup call will happen and throw;
            mockSetup(innerHandler).Returns(TaskConstants.Completed);

            //act
            await act(sut);

            //assert
            await innerHandler.Received(reopenExpected ? 2 : 1).OpenAsync(Arg.Is(false));
            await assert(innerHandler);
            Assert.AreEqual(reopenExpected ? 2 : 1, ctorCallCounter);
        }

        static async Task OpenAsync_ExceptionThrownAndThenSucceed_SuccessfullyOpened(Func<IDelegatingHandler, Task> mockSetup, Func<IDelegatingHandler, Task> act, Func<IDelegatingHandler, Task> assert, Type thrownExceptionType, Type expectedExceptionType)
        {
            int ctorCallCounter = 0;
            var innerHandler = Substitute.For<IDelegatingHandler>();
            var sut = new ErrorDelegatingHandler(() =>
            {
                ctorCallCounter++;
                return innerHandler;
            });

            //set initial operation result that throws

            bool[] setup = { false };
            mockSetup(innerHandler).Returns(ci =>
            {
                if (setup[0])
                {
                    return Task.FromResult(Guid.NewGuid());
                }
                throw ExceptionFactory[thrownExceptionType]();
            });

            //act
            await ((Func<Task>)(() => act(sut))).ExpectedAsync(expectedExceptionType);

            //override outcome
            setup[0] = true;//otherwise previosly setup call will happen and throw;
            mockSetup(innerHandler).Returns(TaskConstants.Completed);

            //act
            await act(sut);

            //assert
            await assert(innerHandler);
            Assert.AreEqual(2, ctorCallCounter);
        }
    }
}