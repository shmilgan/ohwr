helper_hist_sfp_run() {
    rm -f wrs-shmem-*
    run ./hist_sfp_test -v -v -p $TEST_DATA_DIR/$1 -b $TEST_DATA_DIR/$2 2>&1
    echo "run ./hist_sfp_test -v -v -p $TEST_DATA_DIR/$1 -b $TEST_DATA_DIR/$2 2>&1 failed with $status"
    [ "$status" -eq 0 ]
}

helper_dump_shmem_filterout() {
    # skip all fields that are not constant
    cat $1 | grep -v \
		-e "saved_swlifetime" \
		-e "saved_timestamp" \
		-e "ID 4 (\"wrs_hist\"):" \
		-e "[[:digit:]]*\.\." \
	> $1
}

helper_dump_shmem_diff() {
    if [ "$CREATE_RESULTS" = "yes" ]; then
	cp $1 $2
    fi
    echo $1 $2
    run diff $1 $2
    echo "run diff $1 $2 failed with $status"
    [ "$status" -eq 0 ]
}

helper_hist_sfp_run_test() {
    helper_hist_sfp_run "$1" "$2"
    ret=$?
    echo "hist_sfp_run status: $ret"
    if [ "$ret" -ne 0 ]; then return $ret; fi
    $DUMP_SHMEM > dump_out.txt
    helper_dump_shmem_filterout dump_out.txt
    helper_dump_shmem_diff dump_out.txt $RESULT_DIR/"$3"
    return 0
}
