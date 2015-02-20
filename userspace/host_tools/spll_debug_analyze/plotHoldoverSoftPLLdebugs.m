% Maciej Lipinski / CERN / 2015-02-20
% 
% This function is used for analyzing the SoftPLL data when testing holdover. it:
% 1. reads data captured in SoftPLL (using spll_dbg_proxy on switch) and stored by 
%    spll_dbg_read program into files: mPLL.txt, bPLL.txt, hPLL.txt, bPLL0.txt, bPLL1.txt,
%    bPLL2.txt, bPLL3.txt (empty if no data available)
% 2. cleans data from outliers
% 3. detects when switchover/holdover happenend in the data and aligns data to have 
%    this event at the very same moment
% 4. trunkates/extend vectors to be of the same lenght (for ploting) and as specified
%    by the user (i.e. only subset of the data can be displayed) using history_offset
%    and future_offst
%    
% input: 
%       path_name        - string with path to the folder in which *PLL.txt files are stored
%       history_offset_s - the number of seconds that should be displayed before the switchover
%                          happended 
%       future_offset_s  - the number of seconds that should be displayed after switchover
%       backup_n         - how many backup ports were used
%       plotOptions      - option regarding plots
%                          0: (or no argument) show plots 
%                          1: no plots at all
%      outlierOption     - option regarding outlaier removal
%                          0: no removal of outliers
%                          1: outliers      - average-based      : [(avg-threshold*avg) , (avg+threshold*avg)]
%                          2: outliers2     - median/sdev-based  : [median-3*sdev, median+3*sdev]
%                          3: smartOutliers - sdev/avg-based     : [(avg-threshold*sdev) , (avg+threshold*sdev)]

function [mpll, bpll, hpll, switchover] = plotHoldoverSoftPLLdebugs(path_name, history_offset_s, future_offset_s, backup_n, plotOptions, outlierOption)

if ~exist('plotOptions','var')
  plotOptions = 0;
end;

% options

close all;
unitScale = (1/((62.5-(62.5*((2^14)/(1+2^14))))*10^6)); % [s]
if(history_offset_s < 0)
  disp('history_offset max possible');
  history_offset = history_offset_s;
else
  history_offset = ceil(history_offset_s/(unitScale)); % samples
end

if(future_offset_s < 0)
  disp('future_offset max possible');
  future_offset = future_offset_s;
else
  future_offset = ceil(future_offset_s/(unitScale)); % samples
end


disp('-----------------------------------------------------------------------------------------------');
disp('Analyzing SoftPLL dbug data, params:');
disp(sprintf('- path to the analyzed data                    : %s ',path_name));
disp(sprintf('- analyzed number of samples before switchover : %d ',history_offset));
disp(sprintf('- time of analysis before switchover           : %d [s]',history_offset_s));
disp(sprintf('- analyzed number of samples after  switchover : %d ',future_offset));
disp(sprintf('- time of analysis after  switchover           : %d [s]',future_offset_s));
disp(sprintf('- expected number of backup ports              : %d ',backup_n));
disp(sprintf('- plot             (0:show all; 1: no plots)   : %d ',plotOptions));
disp(sprintf('- outliers removal (0:no; 1: ; 2: 3:)          : %d ',outlierOption));
disp('-----------------------------------------------------------------------------------------------');

mpll_tmp   =load('-ascii', sprintf('%s/mPLL.txt' ,path_name), 'data');
bpll_tmp_0 =load('-ascii', sprintf('%s/bPLL.txt' ,path_name), 'data');
bpll_tmp_1 =load('-ascii', sprintf('%s/bPLL1.txt',path_name), 'data');
bpll_tmp_2 =load('-ascii', sprintf('%s/bPLL2.txt',path_name), 'data');
bpll_tmp_3 =load('-ascii', sprintf('%s/bPLL3.txt',path_name), 'data');
hpll_tmp   =load('-ascii', sprintf('%s/hPLL.txt' ,path_name), 'data');

% data read from SoftPLL is arranged as follows (see spll_dbg_read.c):
% column |  1        |  2  |  3  |     4     |    5  |    6  |   7          |      8        |    9   |
%--------|-----------|-----|-----|-----------|-------|-------|--------------|---------------|--------|
% mPLL   | sample_id |  Y  | err | log_y     | err_d | event | avg_err_long | avg_err_short | locked |
% hPLL   | sample_id |  Y  | err | tag       |       | event | avg_err_long | avg_err_short | locked |
% bPLLs  | sample_id |  Y  | err | tag+adder | err_d | event | avg_err_long | avg_err_short | locked |

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%  establish at which row the switchover happened in each backup port data
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
swover_offset_0 = detectSwitchover(bpll_tmp_0,6);% - 2*history_offset;
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
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%  if we take only subset of data, take a bit more of that, otherwise take the shortest history
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
if(history_offset > 0)
  hack_offset_0 = swover_offset_0 - 1.1*history_offset;
  hack_offset_1 = swover_offset_1 - 1.1*history_offset;
  hack_offset_2 = swover_offset_2 - 1.1*history_offset;
  hack_offset_3 = swover_offset_3 - 1.1*history_offset;
else
  min_swover_offset= min([swover_offset_0,swover_offset_1,swover_offset_2,swover_offset_3]);
  hack_offset_0 = swover_offset_0 - min_swover_offset + 1;% + 2^8; % + 2^7 is the l
  hack_offset_1 = swover_offset_1 - min_swover_offset + 1;% + 2^8;
  hack_offset_2 = swover_offset_2 - min_swover_offset + 1;% + 2^8;
  hack_offset_3 = swover_offset_3 - min_swover_offset + 1;% + 2^8;
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%  deal with outliers, since columns have different meaning for different input data,
%  we filter different columns for mPLL, hPLL, bPLLs
%  different types of outlier function can be chosen in by outlierOption, the threshold is
%  defined differently for outlier() and smartOutlier()
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% outliers by fraction of avarage
threshold_vec       = zeros(size(mpll_tmp,2));
threshold_vec(2)=0.5;  % Y
threshold_vec(4)=0.5;  % Y avg log for mPLL only
threshold_vec(7)=2;    % err avg long  mPLL and bPLL
threshold_vec(8)=2;    % err avg short mPLL and bPLL

% outliers by number of standard deviations
smart_threshold_vec   = zeros(size(mpll_tmp,2));
smart_threshold_vec(2)=10; % Y
smart_threshold_vec(4)=10; % long avg for mPLL only %
smart_threshold_vec(7)=10; % err avg long  mPLL and bPLL
smart_threshold_vec(8)=10; % err avg short mPLL and bPLL

%%%%%%%%%%% mPLL %%%%%%%%%%%
mpll_cleared = mpll_tmp(hack_offset_0:end,:);
if (outlierOption == 1)
  mpll_cleared = outliers(mpll_cleared,threshold_vec, 'mpll  ');
end
if (outlierOption == 2)
  mpll_cleared = outliers2(mpll_cleared,threshold_vec, 'mpll  ');
end
if (outlierOption == 3)
  mpll_cleared = smartOutliers(mpll_cleared,smart_threshold_vec, 'mpll smart  ');
end
%%%%%%%%%%% hPLL %%%%%%%%%%%
threshold_vec(4)      = 0;    % tag
threshold_vec(7)      = 0;    % err avg long  mPLL and bPLL
threshold_vec(8)      = 0;    % err avg short mPLL and bPLL
smart_threshold_vec(4)= 0;    % tag
smart_threshold_vec(7)= 0;    % err avg long  mPLL and bPLL
smart_threshold_vec(8)= 0;    % err avg short mPLL and bPLL

hpll_cleared = hpll_tmp(hack_offset_0:end,:);
if (outlierOption == 1)
  hpll_cleared = outliers(hpll_cleared,threshold_vec, 'hpll  ');
end
if (outlierOption == 2)
  hpll_cleared = outliers2(hpll_cleared,threshold_vec, 'hpll  ');
end
if (outlierOption == 3)
  hpll_cleared = smartOutliers(hpll_cleared,smart_threshold_vec, 'hpll smart  ');
end

%%%%%%%%%%% hPLL %%%%%%%%%%%
threshold_vec(7)      = 10;    % err avg long  mPLL and bPLL
threshold_vec(8)      = 10;    % err avg short mPLL and bPLL
smart_threshold_vec(7)= 10;    % err avg long  mPLL and bPLL
smart_threshold_vec(8)= 10;    % err avg short mPLL and bPLL

bpll_cleared_0 = bpll_tmp_0(hack_offset_0:end,:);
if (outlierOption == 1)
  bpll_cleared_0 = outliers(bpll_cleared_0 ,threshold_vec, 'bpll 0');
end
if (outlierOption == 2)
  bpll_cleared_0 = outliers2(bpll_cleared_0 ,threshold_vec, 'bpll 0');
end
if (outlierOption == 3)
  bpll_cleared_0 = smartOutliers(bpll_cleared_0,smart_threshold_vec, 'bpll 0 smart  ');
end

% remove outliers for bPLLs (only outliers() used), re-do detectSwitchover on the shortened 
% vectors because of:
% 1) hisotry_offset
% 2) outlier removal

disp('bPLL0 switchover/holdover');
bpll_switchover_0 = detectSwitchover(bpll_cleared_0,6);
if(backup_n > 1)
  bpll_cleared_1 = bpll_tmp_1(hack_offset_1:end,:);
  if (outlierOption > 0)
    bpll_cleared_1 = outliers(bpll_cleared_1,threshold_vec, 'bpll 1');
  end
  disp('bPLL1 switchover/holdover');
  bpll_switchover_1 = detectSwitchover(bpll_cleared_1,6);
else 
  bpll_switchover_1 = bpll_switchover_0;
end
if(backup_n > 2)
  bpll_cleared_2 = bpll_tmp_2(hack_offset_2:end,:);
  if (outlierOption > 0)
     bpll_cleared_2 = outliers(bpll_cleared_2,threshold_vec, 'bpll 2');
  end
  disp('bPLL2 switchover/holdover');
  bpll_switchover_2 = detectSwitchover(bpll_cleared_2,6);
else 
  bpll_switchover_2 = bpll_switchover_0;
end
if(backup_n > 3)
  bpll_cleared_3 = bpll_tmp_3(hack_offset_3:end,:);
  if (outlierOption > 0)
     bpll_cleared_3 = outliers(bpll_cleared_3,threshold_vec, 'bpll 3');
  end
  disp('bPLL3 switchover/holdover');
  bpll_switchover_3 = detectSwitchover(bpll_cleared_3,6);
else 
  bpll_switchover_3 = bpll_switchover_0;
end

disp('mPLL switchover/holdover');
mpll_switchover = detectSwitchover(mpll_cleared,6);
disp('hPLL switchover/holdover');
hpll_switchover = detectSwitchover(hpll_cleared,6);

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%  get the switchover characteristics
%  the event SWITCHOVER is generated before switching the reference and after, thus, the
%  time it took to switchover can be estimated
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
unitScale = (1/((62.5-(62.5*((2^14)/(1+2^14))))*10^6))*10^3; % [ms]
switchover_length = detectSwitchover(mpll_cleared(mpll_switchover+1:end,:),6);
switchover_time   = switchover_length*unitScale;
disp(sprintf('switchover took %d samples which is %f [ms]',switchover_length,switchover_time));

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%  compose vectors of data of a propor beginning (until switchover):
%  1. if history_offset was required, cut the data
%  2. align the data to have switchover at the same position
% the bPLL data is filled with zeros at the end - just in case  :
%  1. future_offset is longer than data
%  2. there is no bPLL data as there was single backup port and it stopped after switchover 
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
if(history_offset > 0)
  switchover = history_offset;
else
  switchover = min([mpll_switchover,bpll_switchover_0,bpll_switchover_1,bpll_switchover_2,bpll_switchover_3,hpll_switchover]);
end
mpll   = [mpll_cleared(mpll_switchover    -switchover+1:mpll_switchover  -1,:);mpll_cleared(mpll_switchover:end,:)];
hpll   = [hpll_cleared(hpll_switchover    -switchover+1:hpll_switchover  -1,:);hpll_cleared(hpll_switchover:end,:)];
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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%  compose vectors of data of a propor end of the vectors:
%  1. if future_offset was required, cut properly
%  2. if not specified, max possible
% Fill in with zeros bPLL data, if longer future_offset than data
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

tmp_len=length(mpll);
if(future_offset < 0) % if not specified, max possible
 future_offset = length(mpll) - switchover;
end

if(tmp_len - switchover < future_offset)
   mpll = [mpll(:,:);zeros(switchover+future_offset-tmp_len,size(mpll,2))];
elseif(tmp_len - switchover > future_offset)
   mpll = mpll(1:(switchover+future_offset),:);
end

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

% put all bPLLs into a single 3-dimention table (for convenience)
bpll       = bpll_0;
bpll(:,:,2)= bpll_1;
bpll(:,:,3)= bpll_2;
bpll(:,:,4)= bpll_3;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%  some statistics for the user
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
to_ps     = 16000/(2^14); 
bef = 500;
mpll_sdev_b = std(mpll(1:(switchover-bef),5));
if (length(mpll) > 2*switchover)
    mpll_sdev_a =  std(mpll(1:2*switchover,5));
else 
    mpll_sdev_a =  std(mpll(:,5));
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
unitScale = (1/((62.5-(62.5*((2^14)/(1+2^14))))*10^6)); % [s]
len_avg= length(mpll(switchover:end,2));
disp(sprintf('Y control word in holdover mean=%f | sde =%0.1f | samples %0.1f [%0.1f s]', y_mean_dithering, y_sdev_dithering,len_avg ,len_avg*unitScale ));
y_mean_dithering = mean(mpll(1:switchover,2));
y_sdev_dithering = std(mpll(1:switchover,2));
len_avg= length(mpll(1:switchover,2));
disp(sprintf('Y control word before holdover mean=%f | sde =%0.1f  | samples %0.1f [%0.1f s]', y_mean_dithering, y_sdev_dithering,y_avg ,len_avg*unitScale ));

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%  draw plots
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

if(plotOptions==1) % don't draw if using the function as input to other function (when comparing) results from many tests
    disp('-----------------------------------------------------------------------------------------------');
    return
end;

start = 1 ; 
finish= switchover + 1000;
if(finish > length(mpll))
  finish = length(mpll);
end
% fig 1
option = 3; % needed only for draw3() to show also faked and real err
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
end


return

