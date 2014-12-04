% Maciej Lipinski / CERN / 2014-10-22
% 
% scripts to analyzer debugging messages from the SoftPLL of the switch
% 
function output = detectSwitchover(input, column)

size_t = size(input);
output = 1;

%  if size_t(1) == 0
%    return
%  end

for i=1:size_t(1)
  if input(i,column) == 1
    return
  else
    output=output+1;
%      output=output-1;
  end
end

output =0;

return