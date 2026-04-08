# Grid and Routing

Test whenever you touch the connection layer, block bypass, block movement, splice logic, or graph topology.

## Prerequisites

- A preset with at least 3 blocks in a chain (Input > Plugin > Output)

## Test Cases

### TC-GR-001: Connect blocks in a chain

**Steps:**
1. Add an Input, a Plugin block, and an Output to the grid
2. Connect Input to Plugin by dragging from the output port to the input port
3. Connect Plugin to Output

**Expected:** Audio flows through the chain. Route highlights show the signal path when a block is selected.

### TC-GR-002: Mute block breaks route highlight

**Steps:**
1. Select a block in the middle of a chain
2. Set bypass mode to "Mute"
3. Toggle bypass on

**Expected:** Route highlight breaks at the muted block immediately (no need to click away).

### TC-GR-003: Thru bypass preserves route highlight

**Steps:**
1. Select a block in the middle of a chain
2. Set bypass mode to "Thru"
3. Toggle bypass on

**Expected:** Route highlight stays connected through the bypassed block. Audio passes through.

### TC-GR-004: Remove block from middle of chain

**Steps:**
1. Build a chain: Input > A > B > Output
2. Remove block A

**Expected:** Block A is removed. Connections to/from A are cleaned up. Block B is disconnected from Input (no auto-reconnect).

### TC-GR-005: Splice block into existing connection

**Steps:**
1. Build a chain: Input > Output (direct connection)
2. Add a new plugin block by dropping it onto the connection line

**Expected:** New block is spliced in: Input > New > Output. Audio flows through all three.

### TC-GR-006: Move block to new position

**Steps:**
1. Place a block at column 3, row 2
2. Drag it to column 6, row 4

**Expected:** Block moves. Connections follow. Audio continues flowing.

### TC-GR-007: Multiple parallel chains

**Steps:**
1. Add two Input blocks and two Output blocks
2. Create two independent chains: Input1 > Plugin1 > Output1, Input2 > Plugin2 > Output2

**Expected:** Both chains process audio independently. Selecting a block in one chain highlights only that chain's route.
