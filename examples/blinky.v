module top(input clk, rst, output a, b, c);

    reg[20:0] div1;
    always @(posedge clk or negedge rst)
        if(!rst)
            div1 <= 0;
        else
            div1 <= div1 + 1;

    assign a = div1[20];
    assign b = div1[19];

    reg [7:0] div2;
    always @(negedge clk)
        div2 <= div2 + 1;

    assign c = div2[7];

endmodule
