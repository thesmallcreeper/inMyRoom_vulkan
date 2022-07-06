% This file is used to create data for "Curve fitter" matlab tool
% We want to map roughtness and normal length with rational functions

close all;

%% Sample

roughnesses = 0.0:0.01:1.0;
samples = 100000;

z_average = zeros(1, size(roughnesses, 2));
 for i = 1:size(roughnesses, 2)
    roughness = roughnesses(1, i);
    for j = 1:samples
        result = f_GGXxCOSsample(roughness);
        z_average(1, i) = z_average(1, i) + result(1, 3);
    end
    z_average(1, i) = z_average(1, i) / samples;
end

%% Plot
figure(1);
plot(roughnesses, z_average);
