% Maciej Lipinski / CERN / 2014-10-22
% 
% c+p from
% http://stackoverflow.com/questions/20151038/matlab-best-technique-to-remove-outliers-in-data
% 
function output = outliers2(input, threshold_vec, name)

size_t  = size(input);
tmp     = zeros(size_t);
average = mean(input)
tlength = length(threshold_vec);
threshold=zeros(tlength,1);

for j=1:size_t(2)
  if threshold_vec(j) > 0
    threshold(j) = average(j)*threshold_vec(j);
  else
    threshold(j) = 0;
  end
end


for j=1:size_t(2)
  if (threshold_vec(j) == 0)
	continue
  end
  all_idx = 1:length(input(:,j));
  outlier_idx = abs(input(:,j) - median(input(:,j))) > 3*std(input(:,j)); % Find outlier idx
  input(outlier_idx,j) = interp1(all_idx(~outlier_idx), input(~outlier_idx,j), all_idx(outlier_idx)); % Do the same thing for y
end



output =input;

return