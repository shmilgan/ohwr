% Maciej Lipinski / CERN / 2014-10-22
% 
% c+p from
% http://stackoverflow.com/questions/20151038/matlab-best-technique-to-remove-outliers-in-data
% 
% % input:
%      input         - 2-dimentional table
%      threshold_vec - just indicates which column should be handed (non-zero value) and which not (zero)
%      name          - string to be printed
% 
% median/sdev-based : removes values outside of the range:
%                    [median-3*sdev, median+3*sdev]
% the missing values are interpolated

function output = outliers2(input, threshold_vec, name)

size_t  = size(input);
tmp     = zeros(size_t);
average = mean(input);
tlength = length(threshold_vec);
threshold=zeros(tlength,1);

disp('---------------------------------------------------');
disp(sprintf('removing outliers from %s',name));
for j=1:size_t(2)
  if (threshold_vec(j) == 0)
	continue
  end
  all_idx = 1:length(input(:,j));
  outlier_idx = abs(input(:,j) - median(input(:,j))) > 3*std(input(:,j)); % Find outlier idx
  input(outlier_idx,j) = interp1(all_idx(~outlier_idx), input(~outlier_idx,j), all_idx(outlier_idx)); % Do the same thing for y
end

output =input;
disp('---------------------------------------------------');
return