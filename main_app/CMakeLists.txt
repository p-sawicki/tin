add_executable(run_scenarios 
  run_scenarios.cpp
  Logger.cpp
  utils.cpp
)
target_link_libraries(run_scenarios rt)