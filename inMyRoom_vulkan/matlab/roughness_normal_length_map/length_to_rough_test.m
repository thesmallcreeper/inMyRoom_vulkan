% For testing numeric stability

close all;

hold on

bp = 0.999569713502222;
lengths = single(0.66:0.00001:1);

l_a = single(2.097717774247452);
l_b = single(1.972814065483481);
l_p1 = single(-0.657734900252995);
l_p2 = single(1.680396590596468);
l_p3 = single(-1.372370801414914);
l_p4 = single(0.349783989583521);
l_q1 = single(0.473070260173490);
l_q2 = single(-1.347452538251406);
l_q3 = single(1.195834522109090);
l_q4 = single(-0.319730631084095);

r_a = single(4.888243568595819);
r_b = single(3.391651278290348);
r_p1 = single(0.636012472685228);
r_p2 = single(-0.255011254510989);
r_p3 = single(0.058724611967702);
r_p4 = single(-0.439687221231256);
r_q1 = single(-2.054361570093722);
r_q2 = single(-0.263696298954134);
r_q3 = single(0.471329950932046);
r_q4 = single(1.850831473052496);


results = [];
for x = lengths
    if (x < bp)
        a = l_a;
        b = l_b;
        p1 = l_p1;
        p2 = l_p2;
        p3 = l_p3;
        p4 = l_p4;
        q1 = l_q1;
        q2 = l_q2;
        q3 = l_q3;
        q4 = l_q4;
    else
        a = r_a;
        b = r_b;
        p1 = r_p1;
        p2 = r_p2;
        p3 = r_p3;
        p4 = r_p4;
        q1 = r_q1;
        q2 = r_q2;
        q3 = r_q3;
        q4 = r_q4;
    end
    y = (p1*x.^3+p2*x.^2+p3*x + p4)./(q1*x.^3+q2*x.^2+q3*x+q4)+a*sqrt(b*(1-x));
    results = [results, y];
end
plot(lengths, results);

% compare!
plot(z_average, roughnesses);
scatter(z_average, roughnesses);

legend("Function (float accuracy)", "Data (line)", "Data (points)");
hold off