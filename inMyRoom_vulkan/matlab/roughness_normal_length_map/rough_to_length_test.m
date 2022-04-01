% For testing numeric stability

close all;

hold on

bp = 0.1;
f_roughs = single(0:0.001:1);

l_p1 = single(0.035677535055191);
l_p2 = single(5.471493224149564);
l_p3 = single(-1.874217507189248);
l_p4 = single(0.351471287752386);
l_q1 = single(0.150481196938677);
l_q2 = single(5.469239550207198);
l_q3 = single(-1.874197989987550);
l_q4 = single(0.351471256360761);

r_p1 = single(-0.177543621700561);
r_p2 = single(0.673813798126158);
r_p3 = single(0.275914071465718);
r_p4 = single(0.718312055526892);
r_q1 = single(0.677240190094898);
r_q2 = single(0.554530253489181);
r_q3 = single(0.286086912399383);
r_q4 = single(0.717956737567938);

results = [];
for x = f_roughs
    if (x < bp)
        p1 = l_p1;
        p2 = l_p2;
        p3 = l_p3;
        p4 = l_p4;
        q1 = l_q1;
        q2 = l_q2;
        q3 = l_q3;
        q4 = l_q4;
    else
        p1 = r_p1;
        p2 = r_p2;
        p3 = r_p3;
        p4 = r_p4;
        q1 = r_q1;
        q2 = r_q2;
        q3 = r_q3;
        q4 = r_q4;
    end
    y = (p1*x.^3+p2*x.^2+p3*x + p4)./(q1*x.^3+q2*x.^2+q3*x+q4);
    results = [results, y];
end

plot(f_roughs, results);
% compare!
plot(roughnesses, z_average);
scatter(roughnesses, z_average);

legend("Function (float accuracy)", "Data (line)", "Data (points)");
hold off