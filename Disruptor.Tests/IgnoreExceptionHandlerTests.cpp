#include "stdafx.h"

#include "Disruptor/ArgumentException.h"
#include "Disruptor/IgnoreExceptionHandler.h"
#include "StubEvent.h"


using namespace Disruptor;


class IgnoreExceptionHandlerTests : public ::testing::Test
{
};

TEST_F(IgnoreExceptionHandlerTests, ShouldIgnoreException)
{
    auto causeException = ArgumentException("IgnoreExceptionHandler.ShouldIgnoreException");
    auto evt = Tests::StubEvent(0);

    auto exceptionHandler = std::make_shared< IgnoreExceptionHandler< Tests::StubEvent> >();

    EXPECT_NO_THROW(exceptionHandler->handleEventException(causeException, 0L, evt));
}
