#UNSUPPORTED: aarch64
#RUN: export INTEGRATION_TESTS_DIR="%S/../"
#RUN: export LOTTO_BIN="%lotto"
#RUN: export LOTTO_CMD="%stress"
#RUN: bash %s 2>&1

set +e

VELOCITY_TEST="$INTEGRATION_TESTS_DIR/velocity"

# Temporary files to store execution times
WITH_VELOCITY_HANDLER_LOG=$(mktemp)
WITHOUT_VELOCITY_HANDLER_LOG=$(mktemp)

# Number of repetitions for each case
REPEATS=100

echo "Running tests with velocity handler enabled..."
for i in $(seq 1 $REPEATS); do
	start_time=$(date +%s%N)
	$LOTTO_BIN $LOTTO_CMD -s random -- $VELOCITY_TEST >/dev/null 2>&1
	exit_code=$?
	end_time=$(date +%s%N)

	elapsed_time=$((($end_time - $start_time) / 1000000))
	echo $elapsed_time >>$WITH_VELOCITY_HANDLER_LOG
done

echo "Running tests with velocity handler disabled..."
for i in $(seq 1 $REPEATS); do
	start_time=$(date +%s%N)
	$LOTTO_BIN $LOTTO_CMD --handler-task-velocity disable -s random -r 20 -- $VELOCITY_TEST >/dev/null 2>&1
	exit_code=$?
	end_time=$(date +%s%N)

	elapsed_time=$((($end_time - $start_time) / 1000000))
	echo $elapsed_time >>$WITHOUT_VELOCITY_HANDLER_LOG
done

# Processing of results
python3 <<EOF
import numpy as np
from scipy import stats
import sys

with_handler = np.loadtxt("$WITH_VELOCITY_HANDLER_LOG")
without_handler = np.loadtxt("$WITHOUT_VELOCITY_HANDLER_LOG")

# Calculation of statistical significance (simple P-test)
t_stat, p_value = stats.ttest_ind(with_handler, without_handler, equal_var=False)

print("With velocity handler mean time to bug (ms):", np.mean(with_handler))
print("Without velocity handler mean time to bug (ms):", np.mean(without_handler))
print("t-statistic:", t_stat)
print("p-value:", p_value)

if np.mean(with_handler) > np.mean(without_handler):
    print("Decrease of performance.")
    sys.exit(1)

if p_value < 0.05:
    print("Statistically significant improvement with the velocity handler.")
    sys.exit(0)
else:
    print("No statistically significant difference.")
    sys.exit(1)
EOF

PYTHON_EXIT_CODE=$?

rm $WITH_VELOCITY_HANDLER_LOG $WITHOUT_VELOCITY_HANDLER_LOG

exit $PYTHON_EXIT_CODE
