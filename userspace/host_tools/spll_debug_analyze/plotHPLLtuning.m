function [mpll, hpll] = plotHPLLtuning(path_name, start_offset_s, stop_offset_s, option)

% options
% 1 - skip data from mPLL, replace with hPLL - used for analyzing data only from hPLL

close all;

freq = 3184; % Hz
T    =1/freq;% s
unitScale = (1/((62.5-(62.5*((2^14)/(1+2^14))))*10^6)); % [s]
T = unitScale;

if(start_offset_s < 0)
  disp('start_offset max possible');
  start_offset = start_offset_s;
else
  start_offset = ceil(start_offset_s/(T)); % samples
end

if(stop_offset_s < 0)
  disp('stop_offset max possible');
  stop_offset = stop_offset_s;
else
  stop_offset = ceil(stop_offset_s/(T)); % samples
end

mpll_tmp   =load('-ascii', sprintf('%s/mPLL.txt',path_name), 'data');
hpll_tmp   =load('-ascii', sprintf('%s/hPLL.txt',path_name), 'data');

disp('size mpll_tmp');
size(mpll_tmp)
hack_offset_0 = 1;

threshold_vec = zeros(size(hpll_tmp,2));
threshold_vec(2)=0.5;
%  threshold_vec(3)=0.5;
%  threshold_vec(5)=0.5;

hpll_cleared = hpll_tmp;
if(option ==1)
  mpll_cleared = hpll_tmp;
else
  mpll_cleared = mpll_tmp;
end


%  mpll_cleared = outliers(mpll_cleared,threshold_vec, 'mpll');
hpll_cleared = outliers(hpll_cleared,threshold_vec, 'hpll');
%  
%  smart_threshold_vec = zeros(size(hpll_tmp,2));
%  smart_threshold_vec(2)=3; 
%  smart_threshold_vec(3)=3; 
%  smart_threshold_vec(5)=3; 
%  
%  mpll_cleared = smartOutliers(mpll_cleared,smart_threshold_vec, 'mpll');
%  hpll_cleared = smartOutliers(hpll_cleared,smart_threshold_vec, 'hpll');
%  


unitScale = (1/((62.5-(62.5*((2^14)/(1+2^14))))*10^6))*10^3; % [ms]


sz= min([length(mpll_cleared),length(hpll_cleared)]);

mpll   = mpll_cleared(1:sz,:);
disp('size mpll');
size(mpll)
hpll   = hpll_cleared(1:sz,:);

%  drawTune(mpll, hpll, 1, sz , option)

if(start_offset < 0)
    start = 1;
else
    start = start_offset;
end

if(stop_offset < 0 || (start_offset + stop_offset) > sz)
     finish = sz;
else
     finish = start_offset + stop_offset;
end

drawTune(mpll, hpll, start, finish, option)

hpll_tmp(start: finish, [1,2,3, 6])

return

