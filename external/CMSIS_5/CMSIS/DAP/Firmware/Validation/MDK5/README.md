CMSIS-DAP debug unit validation
-------------------------------

The following debug functionality is tested:

- Execution breakpoint with hit count
- Breakpoint on read
- Breakpoint on write
- Memory read
- Memory write
- Register read
- Register write
- Single stepping
- Run/stop debugging

The test is self-contained and can be executed on the hardware target.

To configure the test for a specific hardware target:

1. Open the µVision project and select device mounted on hardware target
   (automatically selects flash algorithm for download).
2. Select CMSIS-DAP as the debugger (if not already selected).
3. Build the project.

To run the test on the hardware target:

1. Connect the CMSIS-DAP debug unit via JTAG/SWD to the hardware target.
2. Connect the CMSIS-DAP debug unit under test to a PC via USB.
3. Open the µVision project and start a debug session.
4. Test results are printed into a `test.log` file.

To run the test on the target in batch mode, open a Command window and execute:
```
C:\> .\test.bat
```

Test results are printed into a `test_results.txt` file.
