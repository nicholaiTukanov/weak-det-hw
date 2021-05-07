`include "../define.vh"

// This table maintains a key value pair that maps acquired locks to the thread working on it.
// A new entry is added when a lock is acquired by a thread and it is deleted when the thread releases the lock.
// Used to record all active locks

module availability_table (clk, rst, release_lock, lock_released, request_lock, lock_requested, lock_granted, emptied_entry);
    input wire clk;
    input wire rst;

    input wire release_lock;   
    input wire [`LOCK_WIDTH-1:0] lock_released;
    input wire request_lock;
    input wire [`LOCK_WIDTH-1:0] lock_requested;
    output logic [`LOCK_WIDTH-1:0] emptied_entry;

    output logic lock_granted;

    logic [`LOCK_WIDTH-1:0] lock0, lock0_nxt, lock1, lock1_nxt;
    logic lock0_state, lock0_state_nxt, lock1_state, lock1_state_nxt;

    register #(.WIDTH(`LOCK_WIDTH), .RESET_VAL(2'b0)) reg_lock0 (.clk(clk), .en(`EN), .rst(`RST), .clear(`CLR), .D(lock0_nxt), .Q(lock0));
    register #(.WIDTH(1)) reg_lock0_state (.clk(clk), .en(`EN), .rst(`RST), .clear(`CLR), .D(lock0_state_nxt), .Q(lock0_state));

    register #(.WIDTH(`LOCK_WIDTH), .RESET_VAL(2'b1)) reg_lock1 (.clk(clk), .en(`EN), .rst(`RST), .clear(`CLR), .D(lock1_nxt), .Q(lock1));
    register #(.WIDTH(1)) reg_lock1_state (.clk(clk), .en(`EN), .rst(`RST), .clear(`CLR), .D(lock1_state_nxt), .Q(lock1_state));

    // register #(.WIDTH(`LOCK_WIDTH)) lock2 (.clk(clk), .en(`EN), .rst(`RST), .clear(`CLR), .D(lock2_nxt), .Q(lock2));
    // register #(.WIDTH(1)) lock2_state (.clk(clk), .en(`EN), .rst(`RST), .clear(`CLR), .D(lock2_state_nxt), .Q(lock2_state));

    // register #(.WIDTH(`LOCK_WIDTH)) lock3 (.clk(clk), .en(`EN), .rst(`RST), .clear(`CLR), .D(lock3_nxt), .Q(lock3));
    // register #(.WIDTH(1)) lock3_state (.clk(clk), .en(`EN), .rst(`RST), .clear(`CLR), .D(lock3_state_nxt), .Q(lock3_state));

    // register #(.WIDTH(`LOCK_WIDTH)) lock4 (.clk(clk), .en(`EN), .rst(`RST), .clear(`CLR), .D(lock4_nxt), .Q(lock4));
    // register #(.WIDTH(1)) lock4_state (.clk(clk), .en(`EN), .rst(`RST), .clear(`CLR), .D(lock4_state_nxt), .Q(lock4_state));

    // register #(.WIDTH(`LOCK_WIDTH)) lock5 (.clk(clk), .en(`EN), .rst(`RST), .clear(`CLR), .D(lock5_nxt), .Q(lock5));
    // register #(.WIDTH(1)) lock5_state (.clk(clk), .en(`EN), .rst(`RST), .clear(`CLR), .D(lock5_state_nxt), .Q(lock5_state));

    // register #(.WIDTH(`LOCK_WIDTH)) lock6 (.clk(clk), .en(`EN), .rst(`RST), .clear(`CLR), .D(lock6_nxt), .Q(lock6));
    // register #(.WIDTH(1)) lock6_state (.clk(clk), .en(`EN), .rst(`RST), .clear(`CLR), .D(lock6_state_nxt), .Q(lock6_state));

    // register #(.WIDTH(`LOCK_WIDTH)) lock7 (.clk(clk), .en(`EN), .rst(`RST), .clear(`CLR), .D(lock7_nxt), .Q(lock7));
    // register #(.WIDTH(1)) lock7_state (.clk(clk), .en(`EN), .rst(`RST), .clear(`CLR), .D(lock7_state_nxt), .Q(lock7_state));

    // register #(.WIDTH(`LOCK_WIDTH)) lock8 (.clk(clk), .en(`EN), .rst(`RST), .clear(`CLR), .D(lock8_nxt), .Q(lock8));
    // register #(.WIDTH(1)) lock8_state (.clk(clk), .en(`EN), .rst(`RST), .clear(`CLR), .D(lock8_state_nxt), .Q(lock8_state));

    // register #(.WIDTH(`LOCK_WIDTH)) lock9 (.clk(clk), .en(`EN), .rst(`RST), .clear(`CLR), .D(lock9_nxt), .Q(lock9));
    // register #(.WIDTH(1)) lock9_state (.clk(clk), .en(`EN), .rst(`RST), .clear(`CLR), .D(lock9_state_nxt), .Q(lock9_state));

    always_comb begin : LockTable
        if (release_lock) begin
            case (lock_released)
                lock0: begin
                    lock0_state_nxt = 1'b0;
                    emptied_entry = lock0;
                end
                lock1: begin
                    lock1_state_nxt = 1'b0;
                    emptied_entry = lock1;
                end
            endcase
        end

        if (request_lock) begin
            case(lock_requested) 
                lock0: begin
                    if (!lock0_state) begin 
                        lock0_state_nxt = 1'b1;
                        lock_granted = 1'b1;
                    end
                    else begin
                        lock0_state_nxt = 1'b0;
                        lock_granted = 1'b0;
                    end
                end
                lock1: begin
                    if (!lock1_state) begin 
                        lock1_state_nxt = 1'b1;
                        lock_granted = 1'b1;
                    end
                    else begin
                        lock1_state_nxt = 1'b0;
                        lock_granted = 1'b0;
                    end
                end
            endcase
        end
    end


endmodule