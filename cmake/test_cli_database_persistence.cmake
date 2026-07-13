if(NOT DEFINED BIOPIC_CLI)
  message(FATAL_ERROR "BIOPIC_CLI is required")
endif()
if(NOT DEFINED BIOPIC_IMAGE)
  message(FATAL_ERROR "BIOPIC_IMAGE is required")
endif()
if(NOT DEFINED BIOPIC_DATABASE)
  message(FATAL_ERROR "BIOPIC_DATABASE is required")
endif()

if(EXISTS "${BIOPIC_DATABASE}")
  file(REMOVE "${BIOPIC_DATABASE}")
endif()

execute_process(
  COMMAND "${BIOPIC_CLI}" database add "${BIOPIC_IMAGE}" --label blocked
          --database "${BIOPIC_DATABASE}"
  RESULT_VARIABLE add_result
  OUTPUT_VARIABLE add_output
  ERROR_VARIABLE add_error
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_STRIP_TRAILING_WHITESPACE)

if(NOT add_result EQUAL 0)
  message(FATAL_ERROR "database add failed (${add_result})\nstdout: ${add_output}\nstderr: ${add_error}")
endif()

if(NOT add_output MATCHES "Fingerprint stored")
  message(FATAL_ERROR "database add did not report success\nstdout: ${add_output}\nstderr: ${add_error}")
endif()

execute_process(
  COMMAND "${BIOPIC_CLI}" scan "${BIOPIC_IMAGE}" --database "${BIOPIC_DATABASE}"
  RESULT_VARIABLE scan_result
  OUTPUT_VARIABLE scan_output
  ERROR_VARIABLE scan_error
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_STRIP_TRAILING_WHITESPACE)

if(NOT scan_result EQUAL 0)
  message(FATAL_ERROR "scan failed (${scan_result})\nstdout: ${scan_output}\nstderr: ${scan_error}")
endif()

if(NOT scan_output MATCHES "Match: exact match")
  message(FATAL_ERROR "scan did not find an exact database match\nstdout: ${scan_output}\nstderr: ${scan_error}")
endif()

if(NOT scan_output MATCHES "Decision:\r?\n  BLOCK")
  message(FATAL_ERROR "scan did not block on persisted fingerprint\nstdout: ${scan_output}\nstderr: ${scan_error}")
endif()
