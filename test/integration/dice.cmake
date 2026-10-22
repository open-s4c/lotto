file(MAKE_DIRECTORY "${WORK}")
set(trace "${WORK}/capture.trace")

function(run phase)
  execute_process(
    COMMAND "${LOTTO}" --load-runtime "${PROBE}" "${phase}"
            --temporary-directory "${WORK}/${phase}" ${ARGN}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
    TIMEOUT 30)
  file(WRITE "${WORK}/${phase}.log" "${stdout}${stderr}")
  if(NOT result STREQUAL "0")
    message(FATAL_ERROR "${phase}: ${result}\n${stdout}${stderr}")
  endif()
  string(REGEX MATCHALL "(HANDLER: [^\r\n]*|THREAD [^\r\n]*)" lines "${stdout}")
  list(LENGTH lines count)
  if(NOT count EQUAL 8)
    message(FATAL_ERROR "Expected four callbacks and threads: ${stdout}")
  endif()
  set(${phase}_lines
      "${lines}"
      PARENT_SCOPE)
endfunction()

run(record -o "${trace}" -- "${PROGRAM}")
run(replay -i "${trace}")
if(NOT record_lines STREQUAL replay_lines)
  message(FATAL_ERROR "Replay changed callback/thread order:\n"
                      "${record_lines}\n${replay_lines}")
endif()
