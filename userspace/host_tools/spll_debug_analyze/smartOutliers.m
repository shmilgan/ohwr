% Maciej Lipinski / CERN / 2014-10-22
% 
% 
% 
function output = smartOutliers(input, threshold_vec, name)

size_t    = size(input)
output    = zeros(size_t);
average   = mean(input)
max_in    = max(input)
min_in    = min(input)
maxmaxmin = max([abs(max(input)-average); abs(average-min(input))])

stdev     = std(input)
tlength   = length(threshold_vec);
threshold=zeros(tlength,1);

for j=1:size_t(2)
  if threshold_vec(j) > 0
%      threshold(j) = maxmaxmin(j)*threshold_vec(j);
    threshold(j) = stdev(j)*threshold_vec(j);
  else
    threshold(j) = 0;
  end
end

% correct all other entries
counter = 0;
cnt=1;
for i=1:size_t(1)
  for j=1:size_t(2)
    if threshold(j) > 0 && ((input(i,j) < (average(j)-threshold(j))))
      output(i,j) = average(j) - threshold(j);
      counter=counter+1;
    elseif threshold(j) > 0 &&  (input(i,j) > (average(j)+threshold(j)))
      output(i,j) = average(j) + threshold(j);
      counter=counter+1;
    else
      output(i,j) = input(i,j);
    end
  end
end

disp(sprintf('%s: removed %d outliers',name, counter));
return