add_executable(RtemsApp)
target_sources(RtemsApp
    PRIVATE     main.c)
target_include_directories(RtemsApp
    PRIVATE     ..)
target_link_libraries(RtemsApp
    PRIVATE     common_build_options
    PRIVATE     SAMV71::Uart
                SAMX71::Runtime::Hal
                SAMX71::Runtime::Monitor
                SAMX71::Runtime::ThreadsCommon
                SAMX71::Runtime::BrokerLock
                SAMX71::Runtime::Mocks
                SAMX71::Runtime::Core
                SAMX71::Runtime::Xdmac)

add_format_target(RtemsApp)
