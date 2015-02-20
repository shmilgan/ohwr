% Maciej Lipinski / CERN / 2014-10-22
% 
% scripts to analyzer debugging messages from the SoftPLL of the switch
% input:
%      input     - 2-dimentional table
%      threshold - vector carring definition of threshold for each column of input table
%                  if value is zero, don't remove outliers
%                  WARNING: column 1 & 6 are never checked
%      name      - string to be printed
% 
% average-based : remove stuff outside of [(avg-threshold*ceil(avg)) , (avg+threshold*ceil(avg))]
%                 if threshold is zero, no outlier removal is performed for the column which 
%                 is far from perfect
% 
% outliers are replaced with previous value in the column
function output = outliers(input, threshold_vec, name)

size_t  = size(input);
tmp     = zeros(size_t);
average = ceil(mean(input));
tlength = length(threshold_vec);
threshold=zeros(tlength,1);

for j=1:size_t(2)
  if threshold_vec(j) > 0
    threshold(j) = abs(average(j)*threshold_vec(j));
  else
    threshold(j) = 0;
  end
end

disp('---------------------------------------------------');
disp(sprintf('removing outliers from %s',name));

% correct first entry
for j=1:size_t(2)
  if threshold(j) > 0 && ((input(1,j) < (average(j)-threshold(j))) || (input(1,j) > (average(j)+threshold(j))))
%      disp(sprintf('i=%d, j=%d: replace %d with %d',i,j,tmp(1,j),average(j)));
    tmp(1,j) = average(j);
  else
    tmp(1,j) = input(1,j);
  end
end

% correct all other entries
counter = 0;
cnt=1;
for i=2:size_t(1)
     
  if(input(i,1) == 0 && input(i,6) == 0)
    continue
  end
     
  for j=1:size_t(2)
    if threshold(j) > 0 && ((input(i,j) < (average(j)-threshold(j))) || (input(i,j) > (average(j)+threshold(j))))
      disp(sprintf('removed sample_id =%d: i=%d, j=%d: replace %d with %d',input(i,1), i,j,input(i,j),input(i-1,j)));
      if(input(i-1,j) ~= 0)
	  tmp(cnt,j) = input(i-1,j);
      elseif(i>2 && input(i-2,j) ~= 0)
	  tmp(cnt,j) = input(i-2,j);
      else
	  tmp(cnt,j) = average(j);
      end
      counter=counter+1;
    else
      tmp(cnt,j) = input(i,j);
    end
  end
  cnt=cnt+1;
end

output =zeros(cnt-1,size_t(2));
output = tmp(1:cnt-1,:);
disp(sprintf('%s: removed %d outliers',name, counter));
disp('---------------------------------------------------');
return