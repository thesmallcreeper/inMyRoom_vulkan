% In order to create needed data for roughness to length curve fit
% Min/Max roughness = 0.01/1.0
% Min/Max length = 0.66/1.0

close all;

close all;

breakpoint_roughness = 0.1;

z_avg_one_to_bp = [];
z_avg_bp_to_zero = [];

roughs_one_to_bp = [];
roughs_bp_to_zero = [];

weights_one_to_bp = [];
weights_bp_to_zero = [];
for i = 1:size(roughnesses,2)
    rough = roughnesses(1, i);
    z_avg = z_average(1, i);
    if (rough > breakpoint_roughness)
         weight = (1-rough).^2 + 0.2;
         weights_one_to_bp = [weights_one_to_bp, weight];
         roughs_one_to_bp = [roughs_one_to_bp, rough];
         z_avg_one_to_bp = [z_avg_one_to_bp, z_avg];
    end
    if (rough < breakpoint_roughness)
         weights_bp_to_zero = [weights_bp_to_zero, 1];
         roughs_bp_to_zero = [roughs_bp_to_zero, rough];
         z_avg_bp_to_zero = [z_avg_bp_to_zero, z_avg];
    end
    if (rough == breakpoint_roughness)
         weights_bp_to_zero = [weights_bp_to_zero, 100];
         roughs_bp_to_zero = [roughs_bp_to_zero, rough];
         z_avg_bp_to_zero = [z_avg_bp_to_zero, z_avg];

         weights_one_to_bp = [weights_one_to_bp, 1000];
         roughs_one_to_bp = [roughs_one_to_bp, rough];
         z_avg_one_to_bp = [z_avg_one_to_bp, z_avg];
    end
end