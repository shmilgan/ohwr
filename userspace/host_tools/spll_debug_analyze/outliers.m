% Maciej Lipinski / CERN / 2014-10-22
% 
% scripts to analyzer debugging messages from the SoftPLL of the switch
% 
function output = outliers(input, threshold_vec, name)

size_t  = size(input)
tmp     = zeros(size_t);
average = mean(input);
tlength = length(threshold_vec);
threshold=zeros(tlength,1);

for j=1:size_t(2)
  if threshold_vec(j) > 0
    threshold(j) = average(j)*threshold_vec(j);
  else
    threshold(j) = 0;
  end
end

disp(threshold);

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
%        disp(sprintf('i=%d, j=%d: replace %d with %d',i,j,input(i,j),input(i-1,j)));
      if(input(i-1,j) == 0)
	  tmp(cnt,j) = input(i-2,j);
      else
	  tmp(cnt,j) = input(i-1,j);
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
return