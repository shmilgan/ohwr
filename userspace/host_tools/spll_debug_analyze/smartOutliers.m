% Maciej Lipinski / CERN / 2014-10-22
% 
% it is not smart, really
% input:
%      input     - 2-dimentional table
%      threshold - vector carring definition of threshold for each column of input table
%                  if value is zero, don't remove outliers
%      name      - string to be printed
% 
% average-based : remove stuff outside of [(avg-threshold*ceil(sdev)) , (avg+threshold*ceil(sdev))]
%                 if threshold is zero, no outlier removal is performed for the column which 
% 
% outliers are replaced with average value
function output = smartOutliers(input, threshold_vec, name)

size_t    = size(input);
output    = zeros(size_t);
average   = mean(input);
max_in    = max(input);
min_in    = min(input);
maxmaxmin = max([abs(max(input)-average); abs(average-min(input))]);

stdev     = std(input);
tlength   = length(threshold_vec);
threshold=zeros(tlength,1);

for j=1:size_t(2)
  if threshold_vec(j) > 0
    threshold(j) = stdev(j)*threshold_vec(j);
  else
    threshold(j) = 0;
  end
end
disp('---------------------------------------------------');
disp(sprintf('removing outliers from %s',name));
% correct all other entries
counter = 0;
cnt=1;
for i=1:size_t(1)
  for j=1:size_t(2)
    if threshold(j) > 0 && ((input(i,j) < (average(j)-threshold(j))))
      disp(sprintf('[i=%d, j=%d] outlier sample_id =%d replace %d with %d',i,j,input(i,1), input(i,j),ceil(average(j))));
      output(i,j) = average(j);% - threshold(j);
      counter=counter+1;
    elseif threshold(j) > 0 &&  (input(i,j) > (average(j)+threshold(j)))
      disp(sprintf('[i=%d, j=%d] outlier sample_id =%d replace %d with %d',i,j,input(i,1), input(i,j),ceil(average(j))));
      output(i,j) = average(j);% + threshold(j);
      counter=counter+1;
    else
      output(i,j) = input(i,j);
    end
  end
end

disp(sprintf('%s: removed %d outliers',name, counter));
disp('---------------------------------------------------');
return