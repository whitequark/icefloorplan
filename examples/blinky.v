module top(input clk, output a, b);

    reg[20:0] div;
    always @(posedge clk)
        div <= div + 1;

    assign a = div[20];
    assign b = div[19];

endmodule
