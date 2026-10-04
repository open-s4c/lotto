# RUN: bash %s %builddir/lotto

python3 - "$1" <<'PYTEST'
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile

lotto = sys.argv[1]
with tempfile.TemporaryDirectory() as tmp:
    def run(code):
        return subprocess.run(
            [lotto, "run", "--no-preload", "-t", tmp, "--",
             sys.executable, "-c", code],
            capture_output=True, timeout=5,
        )

    # Quiet exit and exit after closing both output pipes still return status.
    for setup in ("", "os.close(1); os.close(2);"):
        result = run(f"import os,time; {setup} time.sleep(0.1); os._exit(7)")
        assert result.returncode == 7, result

    # Drain output larger than either pipe's capacity, including the final tail.
    result = run("import sys; sys.stdout.write('o'*262144); "
                 "sys.stderr.write('e'*262144)")
    assert result.returncode == 0, result
    assert result.stdout == b'o' * 262144
    assert result.stderr == b'e' * 262144

    result = run("import os,signal; os.kill(os.getpid(), signal.SIGTERM)")
    assert result.returncode != 0, result

    # A descendant retaining the pipes must not delay reaping the direct child.
    pidfile = Path(tmp) / "grandchild.pid"
    try:
        result = run(
            "import subprocess,sys,pathlib; "
            "p=subprocess.Popen([sys.executable,'-c','import time; time.sleep(30)']); "
            f"pathlib.Path({str(pidfile)!r}).write_text(str(p.pid)); "
            "print('tail',flush=True); sys.exit(7)"
        )
        assert result.returncode == 7, result
        assert result.stdout == b'tail\n', result
    finally:
        if pidfile.exists():
            try:
                os.kill(int(pidfile.read_text()), signal.SIGKILL)
            except ProcessLookupError:
                pass
print("child wait checks passed")
PYTEST
