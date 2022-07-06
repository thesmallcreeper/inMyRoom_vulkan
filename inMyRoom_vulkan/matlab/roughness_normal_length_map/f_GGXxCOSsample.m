function [vector] = f_GGXxCOSsample(roughness)
    a = roughness * roughness;

    u_0 = rand(1);
    u_1 = rand(1);

    cosTheta = sqrt((1. - u_0) / ((a*a - 1.) * u_0 + 1.));
    sinTheta = sqrt(1. - min(cosTheta * cosTheta, 1.));

    cosPhi = cos(2. * pi * u_1);
    sinPhi = sin(2. * pi * u_1);

    vector = [(cosPhi * sinTheta) (sinPhi * sinTheta) (cosTheta)];
end
