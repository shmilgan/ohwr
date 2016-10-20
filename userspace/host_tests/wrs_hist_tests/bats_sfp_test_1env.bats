load bats_sfp_test_config

@test "Check the presence of wrs_dump_shmem" {
    # to know if we have wrs_dump_shmem available
    run command -v $DUMP_SHMEM_CMD
    echo "Please compile $DUMP_SHMEM_CMD"
    [ "$status" -eq 0 ]
}

@test "Check the presence of hist_sfp_test" {
    # to know if we have snmpgetnext available
    run command -v ./hist_sfp_test
    echo "Please compile ./hist_sfp_test"
    [ "$status" -eq 0 ]
}
