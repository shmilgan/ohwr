% plotOptions (optional)
% 0: (or no argument) show plots 
% 1: no plots at all

function [mpll, bpll, hpll, switchover] = plotHoldoverSoftPLLdebugs(path_name, history_offset_s, future_offset_s, backup_n, plotOptions)

if ~exist('plotOptions','var')
  plotOptions = 0;
end;

% options

close all;
freq = 3184; % Hz
T    =1/freq;% s

if(history_offset_s < 0)
  disp('history_offset max possible');
  history_offset = history_offset_s;
else
  history_offset = ceil(history_offset_s/(T)); % samples
end

if(future_offset_s < 0)
  disp('future_offset max possible');
  future_offset = future_offset_s;
else
  future_offset = ceil(future_offset_s/(T)); % samples
end


disp('-----------------------------------------------------------------------------------------------');
disp('Analyzing SoftPLL dbug data, params:');
disp(sprintf('- path to the analyzed data                    : %s ',path_name));
disp(sprintf('- analyzed number of samples before switchover : %d ',history_offset));
disp(sprintf('- time of analysis before switchover           : %d [s]',history_offset_s));
disp(sprintf('- analyzed number of samples after  switchover : %d ',future_offset));
disp(sprintf('- time of analysis after  switchover           : %d [s]',future_offset_s));
disp(sprintf('- expected number of backup ports              : %d ',backup_n));
disp(sprintf('- plot option (0:show all plots; 1: no plots)  : %d ',plotOptions));
disp('-----------------------------------------------------------------------------------------------');

mpll_tmp   =load('-ascii', sprintf('%s/mPLL.txt' ,path_name), 'data');
bpll_tmp_0 =load('-ascii', sprintf('%s/bPLL.txt' ,path_name), 'data');
bpll_tmp_1 =load('-ascii', sprintf('%s/bPLL1.txt',path_name), 'data');
bpll_tmp_2 =load('-ascii', sprintf('%s/bPLL2.txt',path_name), 'data');
bpll_tmp_3 =load('-ascii', sprintf('%s/bPLL3.txt',path_name), 'data');
hpll_tmp   =load('-ascii', sprintf('%s/hPLL.txt' ,path_name), 'data');

swover_offset_0 = detectSwitchover(bpll_tmp_0,6)% - 2*history_offset;
if(backup_n > 1)
  swover_offset_1 = detectSwitchover(bpll_tmp_1,6);% - 2*history_offset;
else
  swover_offset_1 = swover_offset_0; %fake for later calculations
end
if(backup_n > 2)
  swover_offset_2 = detectSwitchover(bpll_tmp_2,6);% - 2*history_offset;
else
  swover_offset_2 = swover_offset_0; %fake for later calculations
end
if(backup_n > 3)
  swover_offset_3 = detectSwitchover(bpll_tmp_3,6);% - 2*history_offset;
else
  swover_offset_3 = swover_offset_0; %fake for later calculations
end

if(history_offset > 0)
  hack_offset_0 = swover_offset_0 - 1.5*history_offset
  hack_offset_1 = swover_offset_1 - 1.5*history_offset;
  hack_offset_2 = swover_offset_2 - 1.5*history_offset;
  hack_offset_3 = swover_offset_3 - 1.5*history_offset;
else
  min_swover_offset= min([swover_offset_0,swover_offset_1,swover_offset_2,swover_offset_3]);
  hack_offset_0 = swover_offset_0 - min_swover_offset + 1;% + 2^8; % + 2^7 is the l
  hack_offset_1 = swover_offset_1 - min_swover_offset + 1;% + 2^8;
  hack_offset_2 = swover_offset_2 - min_swover_offset + 1;% + 2^8;
  hack_offset_3 = swover_offset_3 - min_swover_offset + 1;% + 2^8;
end
%  figure
%  hold on
%  plot(mpll_tmp(:,2));
%  %  plot(mpll_tmp(:,4),'g');

% outliers by fraction of avarage
threshold_vec       = zeros(size(mpll_tmp,2));
threshold_vec(1)=0.5;
threshold_vec(2)=0.5;
threshold_vec(4)=0.5;
threshold_vec(5)=0.5;


% outliers by number of standard deviations
smart_threshold_vec = zeros(size(mpll_tmp,2));
smart_threshold_vec(1)=3;
smart_threshold_vec(2)=9; % Y
smart_threshold_vec(4)=3; % long avg %
smart_threshold_vec(5)=3;

mpll_cleared = outliers(mpll_tmp(hack_offset_0:end,:),threshold_vec, 'mpll  ');
mpll_cleared = smartOutliers(mpll_cleared,smart_threshold_vec, 'mpll smart  ');


disp('size mpll_cleared');
size(mpll_cleared);

threshold_vec(7)=0.5;
smart_threshold_vec(7)=3;
hpll_cleared = outliers(hpll_tmp(hack_offset_0:end,:),threshold_vec, 'hpll  ');
hpll_cleared = smartOutliers(hpll_cleared,smart_threshold_vec, 'hpll smart  ');

bpll_cleared_0    = outliers(bpll_tmp_0(hack_offset_0:end,:),threshold_vec, 'bpll 0');
bpll_cleared_0 = smartOutliers(bpll_cleared_0,smart_threshold_vec, 'bpll 0 smart  ');
bpll_switchover_0 = detectSwitchover(bpll_cleared_0,6)
if(backup_n > 1)
  bpll_cleared_1 = outliers(bpll_tmp_1(hack_offset_1:end,:),threshold_vec, 'bpll 1');
  bpll_switchover_1 = detectSwitchover(bpll_cleared_1,6);
else 
  bpll_switchover_1 = bpll_switchover_0;
end
if(backup_n > 2)
  bpll_cleared_2 = outliers(bpll_tmp_2(hack_offset_2:end,:),threshold_vec, 'bpll 2');
  bpll_switchover_2 = detectSwitchover(bpll_cleared_2,6);
else 
  bpll_switchover_2 = bpll_switchover_0;
end
if(backup_n > 3)
  bpll_cleared_3 = outliers(bpll_tmp_3(hack_offset_3:end,:),threshold_vec, 'bpll 3');
  bpll_switchover_3 = detectSwitchover(bpll_cleared_3,6);
else 
  bpll_switchover_3 = bpll_switchover_0;
end

mpll_switchover = detectSwitchover(mpll_cleared,6)
hpll_switchover = detectSwitchover(hpll_cleared,6)

unitScale = (1/((62.5-(62.5*((2^14)/(1+2^14))))*10^6))*10^3; % [ms]
switchover_length = detectSwitchover(mpll_cleared(mpll_switchover+1:end,:),6);
switchover_time   = ceil(switchover_length*unitScale);
disp(sprintf('switchover took %d samples which is %d [ms]',switchover_length,switchover_time));

if(history_offset > 0)
  switchover = history_offset
else
  switchover = min([mpll_switchover,bpll_switchover_0,bpll_switchover_1,bpll_switchover_2,bpll_switchover_3,hpll_switchover])
end
mpll   = [mpll_cleared(mpll_switchover-switchover+1:mpll_switchover-1,:);mpll_cleared(mpll_switchover:end,:)];
%  disp('size mpll');
%  size(mpll)
hpll   = [hpll_cleared(hpll_switchover-switchover+1:hpll_switchover-1,:);hpll_cleared(hpll_switchover:end,:)];
bpll_0 = [bpll_cleared_0(bpll_switchover_0-switchover+1:bpll_switchover_0-1,:);bpll_cleared_0(bpll_switchover_0+1:end,:);zeros(size(mpll_cleared(mpll_switchover:end,:)))];
if(backup_n > 1)
  bpll_1 = [bpll_cleared_1(bpll_switchover_1-switchover+1:bpll_switchover_1-1,:);bpll_cleared_1(bpll_switchover_1+1:end,:);zeros(size(mpll_cleared(mpll_switchover:end,:)))];
else
  bpll_1 = zeros(size(bpll_0));
end
if(backup_n > 2)
  bpll_2 = [bpll_cleared_2(bpll_switchover_2-switchover+1:bpll_switchover_2-1,:);bpll_cleared_2(bpll_switchover_2+1:end,:);zeros(size(mpll_cleared(mpll_switchover:end,:)))];
else
  bpll_2 = zeros(size(bpll_0));
end
if(backup_n > 3)
  bpll_3 = [bpll_cleared_3(bpll_switchover_3-switchover+1:bpll_switchover_3-1,:);bpll_cleared_3(bpll_switchover_3+1:end,:);zeros(size(mpll_cleared(mpll_switchover:end,:)))];
else
  bpll_3 = zeros(size(bpll_0));
end

tmp_len=length(mpll);
%  [length(mpll), length(hpll)]
if(future_offset < 0)
 future_offset = length(mpll) - switchover
end

if(tmp_len - switchover < future_offset)
   mpll = [mpll(:,:);zeros(switchover+future_offset-tmp_len,size(mpll,2))];
elseif(tmp_len - switchover > future_offset)
   mpll = mpll(1:(switchover+future_offset),:);
end
%  disp('size mpll');
%  size(mpll);
tmp_len = length(bpll_0);
if(tmp_len - switchover < future_offset)
   bpll_0 = [bpll_0;zeros(switchover+future_offset-tmp_len,size(bpll_0,2))];
elseif(tmp_len - switchover > future_offset)
   bpll_0 = bpll_0(1:(switchover+future_offset),:);
end
tmp_len = length(bpll_1);
if(tmp_len - switchover < future_offset)
  bpll_1 = [bpll_1;zeros(switchover+future_offset-tmp_len,size(bpll_1,2))];
elseif(tmp_len - switchover > future_offset)
  bpll_1 = bpll_1(1:(switchover+future_offset),:);
end
tmp_len = length(bpll_2);
if(tmp_len - switchover < future_offset)
  bpll_2 = [bpll_2;zeros(switchover+future_offset-tmp_len,size(bpll_2,2))];
elseif(tmp_len - switchover > future_offset)
  bpll_2 = bpll_2(1:(switchover+future_offset),:);
end
tmp_len = length(bpll_3);
if(tmp_len - switchover < future_offset)
  bpll_3 = [bpll_3;zeros(switchover+future_offset-tmp_len,size(bpll_3,2))];
elseif(tmp_len - switchover > future_offset)
  bpll_3 = bpll_3(1:(switchover+future_offset),:);
end
tmp_len = length(hpll);
if(tmp_len - switchover < future_offset)
   hpll = [hpll;zeros(switchover+future_offset-tmp_len,size(hpll,2))];
elseif(tmp_len  - switchover > future_offset)
   hpll = hpll(1:(switchover+future_offset),:);
end
mpll_switchover = detectSwitchover(mpll,6);
hpll_switchover = detectSwitchover(hpll,6);
bpll_switchover_0 = detectSwitchover(bpll_0,6);
if (backup_n > 1)
  bpll_switchover_1 = detectSwitchover(bpll_1,6);
end
if (backup_n > 2)
  bpll_switchover_2 = detectSwitchover(bpll_2,6);
end
if (backup_n > 3)
  bpll_switchover_3 = detectSwitchover(bpll_3,6);
end
option = 3;

bpll       = bpll_0;
bpll(:,:,2)= bpll_1;
bpll(:,:,3)= bpll_2;
bpll(:,:,4)= bpll_3;

% some statictisc
unitScale = (1/((62.5-(62.5*((2^14)/(1+2^14))))*10^6)); % [s]
to_ps     = 16000/(2^14); 
bef = 500;
mpll_sdev_b = std(mpll(1:(switchover-bef),5));
if (length(mpll) > 2*switchover)
    mpll_sdev_a =  std(mpll(1:2*switchover,5));
else 
    mpll_sdev_a =  std(mpll(:,5))
end
mpll_mte_b = max(mpll(1:(switchover-bef),5))-min(mpll(1:(switchover-bef),5));

if (length(mpll) > 2*switchover)
   mpll_mte_a = max(mpll(1:2*switchover,5))-min(mpll(1:2*switchover,5));
else 
   mpll_mte_a = max(mpll(:,5))-min(mpll(:,5));
end
bpll_mean = mean(bpll_0(1:(switchover-bef),3));
bpll_sdev = std(bpll_0(1:(switchover-bef),3));
bpll_mte  = max(bpll_0(1:(switchover-bef),3))-min(bpll_0(1:(switchover-bef),3));

y_mean_dithering = mean(mpll(switchover:end,2));
y_sdev_dithering = std(mpll(switchover:end,2));
if(switchover > 2^15)
  y_avg            = mpll((switchover-(2^15)),4);
else
  y_avg = -1;
end
%  disp(sprintf('ForTable: MPLL sdev: before=%0.1f => all=%0.1f | MTE: before=%0.1f => all=%0.1f ', mpll_sdev_b, mpll_sdev_a, mpll_mte_b, mpll_mte_a));
%  disp(sprintf('ForTable: BPLL MEAN: before=%0.1f | sdev: before=%0.1f  | MTE: before=%0.1f', bpll_mean, bpll_sdev, bpll_mte));
%  disp(sprintf('ForTable: estimated phase jump mpll=%0.1f | bpll=%0.1f ', max(mpll(1:(switchover+bef),5)), max(bpll_0(1:(switchover),3))));
len_avg= length(mpll(switchover:end,2));
disp(sprintf('Y control word in holdover mean=%f | sde =%0.1f | samples %0.1f [%0.1f s]', y_mean_dithering, y_sdev_dithering,len_avg ,len_avg*T ));



y_mean_dithering = mean(mpll(1:switchover,2));
y_sdev_dithering = std(mpll(1:switchover,2));
len_avg= length(mpll(1:switchover,2));
disp(sprintf('Y control word before holdover mean=%f | sde =%0.1f  | samples %0.1f [%0.1f s]', y_mean_dithering, y_sdev_dithering,y_avg ,len_avg*T ));

if(plotOptions==1)
    disp('-----------------------------------------------------------------------------------------------');
    return
end;

%  bpll = [bpll_0;bpll_1;bpll_2;bpll_3];
start = 1 ; %switchover - 4000;
finish= switchover + 1000;
if(finish > length(mpll))
  finish = length(mpll);
end
% fig 1
draw3(mpll, bpll, hpll, switchover, start, finish, option, backup_n);

if (backup_n > 0)
  start = 1; %switchover - 4000;
  finish= length(mpll);
  draw5(mpll, bpll_0, hpll, switchover, start, finish, option);
  draw4(mpll, bpll_0, hpll, switchover, start, finish, option);
  draw6(mpll, bpll_0, hpll, switchover, start, finish, option);
  
  start = switchover - 400;
  finish= switchover + 400;
  if(start < 1)
    start = 1;
  end
  
%    if(finish > length(mpll))
%      finish = length(mpll);
%    end
%    
%    draw5(mpll, bpll_0, hpll, switchover, start, finish, option);
end


return

