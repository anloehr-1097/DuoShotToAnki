run_tests:
	cmake --build --preset tests --target test_duolib 2>&1 && ctest --preset tests --output-on-failure 2>&1
