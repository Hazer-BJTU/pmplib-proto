#!/bin/bash

prompt() {
    echo "Build options: "
    echo "[-c|--use-cupmp]    <flag>   {off by default}   : enable cupmp lib"
    echo "[-t|--test-targets] <flag>   {off by default}   : generate test targets"
    echo "[-r|--release]      <flag>   {off by default}   : build release"
    echo "[-j|--threads]      <int>    {nproc by default} : number of compile workers"
    echo "[--option-exception-known-only]  <ON/OFF> : only keep known symbols in traceback; ON is recommended."
    echo "[--option-leak-check]            <ON/OFF> : Memory pool leak check that may cause performance degradation. OFF is recommended."
    echo "[--option-workstealing]          <ON/OFF> : only for debug use; OFF is recommended."
    echo "[--option-load-config]           <ON/OFF> : If you want to load configurations from specified source, set it to OFF. ON is recommended."
    echo "[--option-fsm-overload]          <ON/OFF> : Set it to OFF."
    echo "[--option-thead-binding]         <ON/OFF> : Setting this option to ON may result in a deeper call stack. Pay attention to the computation scale. ON is recommended."
    echo "[--option-graphv-debug]          <ON/OFF> : Set it to ON if you want to export & visualize computational DAG."
    echo "[--option-procedure-details]     <ON/OFF> : Set it to ON if you have enabled --option-graphv-debug."
}

USE_CUPMP="OFF"
GENERATE_TEST_TARGETS="OFF"
BUILD_RELEASE="OFF"
ENABLE_PUTILS_GENERAL_EXCEPTION_KNOWN_ONLY="ON"
ENABLE_PUTILS_MEMORY_LEAK_CHECK="OFF"
ENABLE_PUTILS_THREADPOOL_WORKSTEALING_OPTIMIZATION="ON"
ENABLE_MPENGINE_CONFIG_LOAD_DEFAULT="ON"
ENABLE_MPENGINE_FSM_IMPLICIT_OVERLOAD="OFF"
ENABLE_MPENGINE_THREAD_BINDING_OPTIMIZATION="ON"
ENABLE_MPENGINE_GRAPHV_DEBUG_OPTION="ON"
ENABLE_MPENGINE_STORE_PROCEDURE_DETAILS="ON"
THREAD_COUNT=$(nproc)

while [[ $# -gt 0 ]]; do
    case $1 in
        -c|--use-cupmp)
            USE_CUPMP="ON"
            shift 1
            ;;
        -t|--test-targets)
            GENERATE_TEST_TARGETS="ON"
            shift 1
            ;;
        -r|--release)
            BUILD_RELEASE="ON"
            shift 1
            ;;
        -h|--help)
            prompt
            shift 1
            exit 1
            ;;
        -j|--threads)
            THREAD_COUNT="$2"
            shift 2
            ;;
        --option-exception-known-only)
            ENABLE_PUTILS_GENERAL_EXCEPTION_KNOWN_ONLY="$2"
            shift 2
            ;;
        --option-leak-check)
            ENABLE_PUTILS_MEMORY_LEAK_CHECK="$2"
            shift 2
            ;;
        --option-workstealing)
            ENABLE_PUTILS_THREADPOOL_WORKSTEALING_OPTIMIZATION="$2"
            shift 2
            ;;
        --option-load-config)
            ENABLE_MPENGINE_CONFIG_LOAD_DEFAULT="$2"
            shift 2
            ;;
        --option-fsm-overload)
            ENABLE_MPENGINE_FSM_IMPLICIT_OVERLOAD="$2"
            shift 2
            ;;
        --option-thread-binding)
            ENABLE_MPENGINE_THREAD_BINDING_OPTIMIZATION="$2"
            shift 2
            ;;
        --option-graphv-debug)
            ENABLE_MPENGINE_GRAPHV_DEBUG_OPTION="$2"
            shift 2
            ;;
        --option-procedure-details)
            ENABLE_MPENGINE_STORE_PROCEDURE_DETAILS="$2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1."
            prompt
            exit 1
            ;;
    esac
done

rm -rf bin/*
touch bin/.bin
rm -rf build/*
touch build/.build
rm -rf lib/*
touch lib/.lib

cmake -B build \
-DUSE_CUPMP="$USE_CUPMP" \
-DGENERATE_TEST_TARGETS="$GENERATE_TEST_TARGETS" \
-DBUILD_RELEASE="$BUILD_RELEASE" \
-DENABLE_PUTILS_GENERAL_EXCEPTION_KNOWN_ONLY="$ENABLE_PUTILS_GENERAL_EXCEPTION_KNOWN_ONLY" \
-DENABLE_PUTILS_MEMORY_LEAK_CHECK="$ENABLE_PUTILS_MEMORY_LEAK_CHECK" \
-DENABLE_PUTILS_THREADPOOL_WORKSTEALING_OPTIMIZATION="$ENABLE_PUTILS_THREADPOOL_WORKSTEALING_OPTIMIZATION" \
-DENABLE_MPENGINE_CONFIG_LOAD_DEFAULT="$ENABLE_MPENGINE_CONFIG_LOAD_DEFAULT" \
-DENABLE_MPENGINE_FSM_IMPLICIT_OVERLOAD="$ENABLE_MPENGINE_FSM_IMPLICIT_OVERLOAD" \
-DENABLE_MPENGINE_THREAD_BINDING_OPTIMIZATION="$ENABLE_MPENGINE_THREAD_BINDING_OPTIMIZATION" \
-DENABLE_MPENGINE_GRAPHV_DEBUG_OPTION=ON="$ENABLE_MPENGINE_GRAPHV_DEBUG_OPTION" \
-DENABLE_MPENGINE_STORE_PROCEDURE_DETAILS="$ENABLE_MPENGINE_STORE_PROCEDURE_DETAILS"

cmake --build build -j"$THREAD_COUNT"

export PATH=$(pwd)/bin/pytools:$PATH
