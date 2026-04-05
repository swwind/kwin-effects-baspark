cmake_minimum_required(VERSION 3.16)

project(kwin-effect-baspark)

set(KF_MIN_VERSION "6.0.0")
set(QT_MIN_VERSION "6.6.0")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)
include(ECMQtDeclareLoggingCategory)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core Gui OpenGL)
find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS Config CoreAddons)
find_package(KWin REQUIRED)

add_definitions(-DQT_NO_KEYWORDS)

set(baspark_SOURCES
    main.cpp
    baspark.cpp
)

kcoreaddons_add_plugin(baspark
    SOURCES ${baspark_SOURCES}
    INSTALL_NAMESPACE "kwin/effects/plugins"
)

target_link_libraries(baspark PRIVATE
    KWin::kwin
    Qt6::Gui
    Qt6::OpenGL
    KF6::ConfigCore
    KF6::CoreAddons
)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
