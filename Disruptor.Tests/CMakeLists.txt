cmake_minimum_required(VERSION 3.20)
project(Disruptor.Tests CXX)

# Google Test for testing framework
find_package(GTest REQUIRED)

set(DisruptorTests_sources

    main.cpp
    
    AggregateEventHandlerTests.cpp
    AggregateEventHandlerTestsFixture.cpp
    BatchEventProcessorTests.cpp
    BatchEventProcessorTestsFixture.cpp
    BatchingTests.cpp
    BusySpinWaitStrategyTests.cpp
    ConsumerRepositoryTests.cpp
    ConsumerRepositoryTestsFixture.cpp
    DelayedEventHandler.cpp
    DisruptorFixture.cpp
    DisruptorStressTest.cpp
    DisruptorTests.cpp
    DummySequenceBarrier.cpp
    EventPollerTests.cpp
    EventPublisherTests.cpp
    ExceptionThrowingEventHandler.cpp
    FatalExceptionHandlerTests.cpp
    FixedSequenceGroupTest.cpp
    IgnoreExceptionHandlerTests.cpp
    LifecycleAwareTests.cpp
    MultiProducerSequencerTests.cpp
    RingBufferTests.cpp
    RingBufferTestsFixture.cpp
    RingBufferWithMocksTest.cpp
    SequenceBarrierTests.cpp
    SequenceBarrierTestsFixture.cpp
    SequenceReportingCallbackTests.cpp
    SequencerTests.cpp
    SequenceUpdater.cpp
    SleepingEventHandler.cpp
    SleepingWaitStrategyTests.cpp
    SpinWaitWaitStrategyTests.cpp
    StubEvent.cpp
    StubExecutor.cpp
    StubPublisher.cpp
    TestWaiter.cpp
    TestWorkHandler.cpp
    TimeoutBlockingWaitStrategyTests.cpp
    UtilTests.cpp
    WaitStrategyTestUtil.cpp
    WorkerPoolTests.cpp
    YieldingWaitStrategyTests.cpp
)

include_directories("..")

add_executable(Disruptor.Tests ${DisruptorTests_sources})

target_link_libraries(Disruptor.Tests
	DisruptorStatic
	Disruptor.TestTools
	GTest::gtest
	GTest::gtest_main
	GTest::gmock
	GTest::gmock_main
)

add_test(cmake_Disruptor.Tests ${CMAKE_CURRENT_BINARY_DIR}/Disruptor.Tests --result_code=no --report_level=no)

