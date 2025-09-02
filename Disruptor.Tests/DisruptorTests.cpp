#include "stdafx.h"

#include <any>

#include "Disruptor/Disruptor.h"

#include "Disruptor.TestTools/CountdownEvent.h"

#include "AtomicReference.h"
#include "DisruptorFixture.h"
#include "EventHandlerStub.h"
#include "ExceptionThrowingEventHandler.h"
#include "SleepingEventHandler.h"
#include "StubExceptionHandler.h"
#include "StubPublisher.h"


using namespace Disruptor;
using namespace ::Disruptor::Tests;



TEST_F(DisruptorFixture, ShouldCreateEventProcessorGroupForFirstEventProcessors)
{
    m_executor->ignoreExecutions();
    auto eventHandler1 = std::make_shared< SleepingEventHandler >();
    auto eventHandler2 = std::make_shared< SleepingEventHandler >();

    auto eventHandlerGroup = m_disruptor->handleEventsWith({ eventHandler1, eventHandler2 });
    m_disruptor->start();

    EXPECT_TRUE(eventHandlerGroup != nullptr);
    EXPECT_EQ(m_executor->getExecutionCount(), 2);
}

TEST_F(DisruptorFixture, ShouldMakeEntriesAvailableToFirstHandlersImmediately)
{
    auto countDownLatch = std::make_shared< CountdownEvent >(2);
    auto eventHandler = std::make_shared< EventHandlerStub< TestEvent > >(countDownLatch);

    m_disruptor->handleEventsWith({ createDelayedEventHandler(), eventHandler });

    ensureTwoEventsProcessedAccordingToDependencies(countDownLatch);
}

TEST_F(DisruptorFixture, ShouldWaitUntilAllFirstEventProcessorsProcessEventBeforeMakingItAvailableToDependentEventProcessors)
{
    auto eventHandler1 = createDelayedEventHandler();

    auto countDownLatch = std::make_shared< CountdownEvent >(2);
    auto eventHandler2 = std::make_shared< EventHandlerStub< TestEvent > >(countDownLatch);

    m_disruptor->handleEventsWith(eventHandler1)->then(eventHandler2);

    ensureTwoEventsProcessedAccordingToDependencies(countDownLatch, { eventHandler1 });
}

TEST_F(DisruptorFixture, ShouldAllowSpecifyingSpecificEventProcessorsToWaitFor)
{
    auto handler1 = createDelayedEventHandler();
    auto handler2 = createDelayedEventHandler();

    auto countDownLatch = std::make_shared< CountdownEvent >(2);
    auto handlerWithBarrier = std::make_shared< EventHandlerStub< TestEvent > >(countDownLatch);

    m_disruptor->handleEventsWith({ handler1, handler2 });
    m_disruptor->after({ handler1, handler2 })->handleEventsWith(handlerWithBarrier);

    ensureTwoEventsProcessedAccordingToDependencies(countDownLatch, { handler1, handler2 });
}

TEST_F(DisruptorFixture, ShouldWaitOnAllProducersJoinedByAnd)
{
    auto handler1 = createDelayedEventHandler();
    auto handler2 = createDelayedEventHandler();

    auto countDownLatch = std::make_shared< CountdownEvent >(2);
    auto handlerWithBarrier = std::make_shared< EventHandlerStub< TestEvent > >(countDownLatch);

    m_disruptor->handleEventsWith(handler1);
    auto handler2Group = m_disruptor->handleEventsWith(handler2);
    m_disruptor->after(handler1)->And(handler2Group)->handleEventsWith(handlerWithBarrier);

    ensureTwoEventsProcessedAccordingToDependencies(countDownLatch, { handler1, handler2 });
}

TEST_F(DisruptorFixture, ShouldThrowExceptionIfHandlerIsNotAlreadyConsuming)
{
    EXPECT_THROW(m_disruptor->after(createDelayedEventHandler())->handleEventsWith(createDelayedEventHandler()), ArgumentException);
}

TEST_F(DisruptorFixture, ShouldSupportSpecifyingAExceptionHandlerForEventProcessors)
{
    auto eventHandled = std::make_shared< AtomicReference< std::exception > >();
    auto exceptionHandler = std::make_shared< StubExceptionHandler< TestEvent > >(eventHandled);
    auto testException = std::exception();
    auto handler = std::make_shared< ExceptionThrowingEventHandler >(testException);

    m_disruptor->handleExceptionsWith(exceptionHandler);
    m_disruptor->handleEventsWith(handler);

    publishEvent();

    auto actualException = waitFor(eventHandled);
    EXPECT_TRUE(testException.what() == actualException.what());
}

TEST_F(DisruptorFixture, ShouldOnlyApplyExceptionsHandlersSpecifiedViaHandleExceptionsWithOnNewEventProcessors)
{
    auto eventHandled = std::make_shared< AtomicReference< std::exception > >();
    auto exceptionHandler = std::make_shared< StubExceptionHandler< TestEvent > >(eventHandled);
    auto testException = std::exception();
    auto handler = std::make_shared< ExceptionThrowingEventHandler >(testException);

    m_disruptor->handleExceptionsWith(exceptionHandler);
    m_disruptor->handleEventsWith(handler);
    m_disruptor->handleExceptionsWith(std::make_shared< FatalExceptionHandler< TestEvent > >());

    publishEvent();

    auto actualException = waitFor(eventHandled);
    EXPECT_TRUE(testException.what() == actualException.what());
}

TEST_F(DisruptorFixture, ShouldSupportSpecifyingADefaultExceptionHandlerForEventProcessors)
{
    auto eventHandled = std::make_shared< AtomicReference< std::exception > >();
    auto exceptionHandler = std::make_shared< StubExceptionHandler< TestEvent > >(eventHandled);
    auto testException = std::exception();
    auto handler = std::make_shared< ExceptionThrowingEventHandler >(testException);

    m_disruptor->setDefaultExceptionHandler(exceptionHandler);
    m_disruptor->handleEventsWith(handler);

    publishEvent();

    auto actualException = waitFor(eventHandled);
    EXPECT_TRUE(testException.what() == actualException.what());
}

TEST_F(DisruptorFixture, ShouldApplyDefaultExceptionHandlerToExistingEventProcessors)
{
    auto eventHandled = std::make_shared< AtomicReference< std::exception > >();
    auto exceptionHandler = std::make_shared< StubExceptionHandler< TestEvent > >(eventHandled);
    auto testException = std::exception();
    auto handler = std::make_shared< ExceptionThrowingEventHandler >(testException);

    m_disruptor->handleEventsWith(handler);
    m_disruptor->setDefaultExceptionHandler(exceptionHandler);

    publishEvent();

    auto actualException = waitFor(eventHandled);
    EXPECT_TRUE(testException.what() == actualException.what());
}

TEST_F(DisruptorFixture, ShouldBlockProducerUntilAllEventProcessorsHaveAdvanced)
{
    auto delayedEventHandler = createDelayedEventHandler();
    m_disruptor->handleEventsWith(delayedEventHandler);

    auto ringBuffer = m_disruptor->start();
    delayedEventHandler->awaitStart();

    auto stubPublisher = std::make_shared< StubPublisher >(ringBuffer);
    try
    {
        m_executor->execute([stubPublisher] { stubPublisher->run(); });
        
        assertProducerReaches(stubPublisher, 4, true);

        delayedEventHandler->processEvent();
        delayedEventHandler->processEvent();
        delayedEventHandler->processEvent();
        delayedEventHandler->processEvent();
        delayedEventHandler->processEvent();

        assertProducerReaches(stubPublisher, 5, false);
    }
    catch (...)
    {
    }

    stubPublisher->halt();
}

TEST_F(DisruptorFixture, ShouldBeAbleToOverrideTheExceptionHandlerForAEventProcessor)
{
    auto testException = std::exception();
    auto eventHandler = std::make_shared< ExceptionThrowingEventHandler >(testException);
    m_disruptor->handleEventsWith(eventHandler);

    auto reference = std::make_shared< AtomicReference< std::exception > >();
    auto exceptionHandler = std::make_shared< StubExceptionHandler< TestEvent > >(reference);

    m_disruptor->handleExceptionsFor(eventHandler)->with(exceptionHandler);

    publishEvent();

    waitFor(reference);
}

TEST_F(DisruptorFixture, ShouldThrowExceptionWhenAddingEventProcessorsAfterTheProducerBarrierHasBeenCreated)
{
    m_executor->ignoreExecutions();
    m_disruptor->handleEventsWith(std::make_shared< SleepingEventHandler >());
    m_disruptor->start();
    EXPECT_THROW(m_disruptor->handleEventsWith(std::make_shared< SleepingEventHandler >()), InvalidOperationException);
}

TEST_F(DisruptorFixture, ShouldThrowExceptionIfStartIsCalledTwice)
{
    m_executor->ignoreExecutions();
    m_disruptor->handleEventsWith(std::make_shared< SleepingEventHandler >());
    m_disruptor->start();
    EXPECT_THROW(m_disruptor->start(), InvalidOperationException);
}

TEST_F(DisruptorFixture, ShouldSupportCustomProcessorsAsDependencies)
{
    auto&& ringBuffer = m_disruptor->ringBuffer();

    auto delayedEventHandler = createDelayedEventHandler();

    auto countDownLatch = std::make_shared< CountdownEvent >(2);
    auto handlerWithBarrier = std::make_shared< EventHandlerStub< TestEvent > >(countDownLatch);

    auto processor = std::make_shared< BatchEventProcessor< TestEvent > >(ringBuffer, ringBuffer->newBarrier(), delayedEventHandler);
    m_disruptor->handleEventsWith(processor)->then(handlerWithBarrier);

    ensureTwoEventsProcessedAccordingToDependencies(countDownLatch, { delayedEventHandler });
}

TEST_F(DisruptorFixture, ShouldSupportHandlersAsDependenciesToCustomProcessors)
{
    auto delayedEventHandler = createDelayedEventHandler();
    m_disruptor->handleEventsWith(delayedEventHandler);

    auto ringBuffer = m_disruptor->ringBuffer();
    auto countDownLatch = std::make_shared< CountdownEvent >(2);
    auto handlerWithBarrier = std::make_shared< EventHandlerStub< TestEvent > >(countDownLatch);

    auto sequenceBarrier = m_disruptor->after(delayedEventHandler)->asSequenceBarrier();
    auto processor = std::make_shared< BatchEventProcessor< TestEvent > >(ringBuffer, sequenceBarrier, handlerWithBarrier);
    m_disruptor->handleEventsWith(processor);

    ensureTwoEventsProcessedAccordingToDependencies(countDownLatch, { delayedEventHandler });
}

TEST_F(DisruptorFixture, ShouldSupportCustomProcessorsAndHandlersAsDependencies)
{
    auto delayedEventHandler1 = createDelayedEventHandler();
    auto delayedEventHandler2 = createDelayedEventHandler();
    m_disruptor->handleEventsWith(delayedEventHandler1);

    auto ringBuffer = m_disruptor->ringBuffer();
    auto countDownLatch = std::make_shared< CountdownEvent >(2);
    auto handlerWithBarrier = std::make_shared< EventHandlerStub< TestEvent > >(countDownLatch);

    auto sequenceBarrier = m_disruptor->after(delayedEventHandler1)->asSequenceBarrier();
    auto processor = std::make_shared< BatchEventProcessor< TestEvent > >(ringBuffer, sequenceBarrier, delayedEventHandler2);

    m_disruptor->after(delayedEventHandler1)->And(processor)->handleEventsWith(handlerWithBarrier);

    ensureTwoEventsProcessedAccordingToDependencies(countDownLatch, { delayedEventHandler1, delayedEventHandler2 });
}

TEST_F(DisruptorFixture, ShouldProvideEventsToWorkHandlers)
{
    auto workHandler1 = createTestWorkHandler();
    auto workHandler2 = createTestWorkHandler();
    m_disruptor->handleEventsWithWorkerPool({ workHandler1, workHandler2 });

    publishEvent();
    publishEvent();

    workHandler1->processEvent();
    workHandler2->processEvent();
}

TEST_F(DisruptorFixture, ShouldSupportUsingWorkerPoolAsDependency)
{
    auto workHandler1 = createTestWorkHandler();
    auto workHandler2 = createTestWorkHandler();
    auto delayedEventHandler = createDelayedEventHandler();
    m_disruptor->handleEventsWithWorkerPool({ workHandler1, workHandler2 })->then(delayedEventHandler);

    publishEvent();
    publishEvent();

    EXPECT_EQ(m_disruptor->getBarrierFor(delayedEventHandler)->cursor(), -1L);

    workHandler2->processEvent();
    workHandler1->processEvent();

    delayedEventHandler->processEvent();
}

TEST_F(DisruptorFixture, ShouldSupportUsingWorkerPoolAsDependencyAndProcessFirstEventAsSoonAsItIsAvailable)
{
    auto workHandler1 = createTestWorkHandler();
    auto workHandler2 = createTestWorkHandler();
    auto delayedEventHandler = createDelayedEventHandler();
    m_disruptor->handleEventsWithWorkerPool({ workHandler1, workHandler2 })->then(delayedEventHandler);

    publishEvent();
    publishEvent();

    workHandler1->processEvent();
    delayedEventHandler->processEvent();

    workHandler2->processEvent();
    delayedEventHandler->processEvent();
}

TEST_F(DisruptorFixture, ShouldSupportUsingWorkerPoolWithADependency)
{
    auto workHandler1 = createTestWorkHandler();
    auto workHandler2 = createTestWorkHandler();
    auto delayedEventHandler = createDelayedEventHandler();
    m_disruptor->handleEventsWith(delayedEventHandler)->thenHandleEventsWithWorkerPool({ workHandler1, workHandler2 });

    publishEvent();
    publishEvent();

    delayedEventHandler->processEvent();
    delayedEventHandler->processEvent();

    workHandler1->processEvent();
    workHandler2->processEvent();
}

TEST_F(DisruptorFixture, ShouldSupportCombiningWorkerPoolWithEventHandlerAsDependencyWhenNotPreviouslyRegistered)
{
    auto workHandler1 = createTestWorkHandler();
    auto delayedEventHandler1 = createDelayedEventHandler();
    auto delayedEventHandler2 = createDelayedEventHandler();
    m_disruptor->handleEventsWith(delayedEventHandler1)->And(m_disruptor->handleEventsWithWorkerPool(workHandler1))->then(delayedEventHandler2);

    publishEvent();
    publishEvent();

    delayedEventHandler1->processEvent();
    delayedEventHandler1->processEvent();

    workHandler1->processEvent();
    delayedEventHandler2->processEvent();

    workHandler1->processEvent();
    delayedEventHandler2->processEvent();
}

TEST_F(DisruptorFixture, ShouldThrowTimeoutExceptionIfShutdownDoesNotCompleteNormally)
{
    auto delayedEventHandler = createDelayedEventHandler();
    m_disruptor->handleEventsWith(delayedEventHandler);
    publishEvent();

    EXPECT_THROW(m_disruptor->shutdown(std::chrono::seconds(1)), TimeoutException);
}

TEST_F(DisruptorFixture, ShouldTrackRemainingCapacity)
{
    std::vector< std::shared_ptr< std::int64_t > > remainingCapacity = { std::make_shared< std::int64_t >(-1) };
    auto eventHandler = std::make_shared< TempEventHandler >(m_disruptor, remainingCapacity);

    m_disruptor->handleEventsWith(eventHandler);

    publishEvent();

    while (*remainingCapacity[0] == -1)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    EXPECT_EQ(*remainingCapacity[0], m_ringBuffer->bufferSize() - 1L);
    EXPECT_EQ(m_disruptor->ringBuffer()->getRemainingCapacity(), m_ringBuffer->bufferSize() - 0L);
}

TEST_F(DisruptorFixture, ShouldAllowEventHandlerWithSuperType)
{
    auto latch = std::make_shared< CountdownEvent >(2);
    auto objectHandler = std::make_shared< EventHandlerStub< TestEvent > >(latch);

    m_disruptor->handleEventsWith(objectHandler);

    ensureTwoEventsProcessedAccordingToDependencies(latch);
}

TEST_F(DisruptorFixture, ShouldAllowChainingEventHandlersWithSuperType)
{
    auto latch = std::make_shared< CountdownEvent >(2);
    auto delayedEventHandler = createDelayedEventHandler();
    auto objectHandler = std::make_shared< EventHandlerStub< TestEvent > >(latch);

    m_disruptor->handleEventsWith(delayedEventHandler)->then(objectHandler);

    ensureTwoEventsProcessedAccordingToDependencies(latch, { delayedEventHandler });
}

TEST_F(DisruptorFixture, ShouldMakeEntriesAvailableToFirstCustomProcessorsImmediately)
{
    auto countDownLatch = std::make_shared< CountdownEvent >(2);
    auto eventHandler = std::make_shared< EventHandlerStub< TestEvent > >(countDownLatch);

    m_disruptor->handleEventsWith(std::make_shared< EventProcessorFactory >(m_disruptor, eventHandler, 0));

    ensureTwoEventsProcessedAccordingToDependencies(countDownLatch);
}

TEST_F(DisruptorFixture, ShouldHonourDependenciesForCustomProcessors)
{
    auto countDownLatch = std::make_shared< CountdownEvent >(2);
    auto eventHandler = std::make_shared< EventHandlerStub< TestEvent > >(countDownLatch);
    auto delayedEventHandler = createDelayedEventHandler();

    m_disruptor->handleEventsWith(delayedEventHandler)->then(std::make_shared< EventProcessorFactory >(m_disruptor, eventHandler, 1));

    ensureTwoEventsProcessedAccordingToDependencies(countDownLatch, { delayedEventHandler });
}

