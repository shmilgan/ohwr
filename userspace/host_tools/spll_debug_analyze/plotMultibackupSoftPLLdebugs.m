function [mpll, bpll, hpll, switchover] = plotSoftPLLdebugs(path_name, history_offset, future_offset, backup_n)

% options

close all;

if(history_offset <5000)
  disp('history_offset argument too small, make it min 5000')
  return
end

mpll_tmp   =load('-ascii', sprintf('%s/mPLL.txt',path_name), 'data');
bpll_tmp_0 =load('-ascii', sprintf('%s/bPLL.txt',path_name), 'data');
bpll_tmp_1 =load('-ascii', sprintf('%s/bPLL1.txt',path_name), 'data');
bpll_tmp_2 =load('-ascii', sprintf('%s/bPLL2.txt',path_name), 'data');
bpll_tmp_3 =load('-ascii', sprintf('%s/bPLL3.txt',path_name), 'data');
hpll_tmp   =load('-ascii', sprintf('%s/hPLL.txt',path_name), 'data');

disp('size mpll_tmp');
size(mpll_tmp)
hack_offset_0 = detectSwitchover(bpll_tmp_0,6) - 2*history_offset
if(backup_n > 1)
  hack_offset_1 = detectSwitchover(bpll_tmp_1,6) - 2*history_offset
end
if(backup_n > 2)
  hack_offset_2 = detectSwitchover(bpll_tmp_2,6) - 2*history_offset
end
if(backup_n > 3)
  hack_offset_3 = detectSwitchover(bpll_tmp_3,6) - 2*history_offset
end

threshold_vec = zeros(size(mpll_tmp,2));
threshold_vec(1)=0.5;
threshold_vec(2)=0.5;
threshold_vec(4)=0.5;
threshold_vec(5)=0.5;
mpll_cleared = outliers(mpll_tmp(hack_offset_0:end,:),threshold_vec, 'mpll');
disp('size mpll_cleared');
size(mpll_cleared)

hpll_cleared = outliers(hpll_tmp(hack_offset_0:end,:),threshold_vec, 'hpll');

bpll_cleared_0    = outliers(bpll_tmp_0(hack_offset_0:end,:),threshold_vec, 'bpll 0');
bpll_switchover_0 = detectSwitchover(bpll_cleared_0,6);
if(backup_n > 1)
  bpll_cleared_1 = outliers(bpll_tmp_1(hack_offset_1:end,:),threshold_vec, 'bpll 1');
  bpll_switchover_1 = detectSwitchover(bpll_cleared_1,6);
end
if(backup_n > 2)
  bpll_cleared_2 = outliers(bpll_tmp_2(hack_offset_2:end,:),threshold_vec, 'bpll 2');
  bpll_switchover_2 = detectSwitchover(bpll_cleared_2,6);
end
if(backup_n > 3)
  bpll_cleared_3 = outliers(bpll_tmp_3(hack_offset_3:end,:),threshold_vec, 'bpll 3');
  bpll_switchover_3 = detectSwitchover(bpll_cleared_3,6);
end

mpll_switchover = detectSwitchover(mpll_cleared,6);
hpll_switchover = detectSwitchover(hpll_cleared,6);

unitScale = (1/((62.5-(62.5*((2^14)/(1+2^14))))*10^6))*10^3; % [ms]
switchover_length = detectSwitchover(mpll_cleared(mpll_switchover+1:end,:),6);
switchover_time   = ceil(switchover_length*unitScale);
disp(sprintf('switchover took %d samples which is %d [ms]',switchover_length,switchover_time));

switchover = history_offset;

mpll   = [mpll_cleared(mpll_switchover-switchover+1:mpll_switchover-1,:);mpll_cleared(mpll_switchover:end,:)];
disp('size mpll');
size(mpll)
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
if(future_offset < 0)
    future_offset = min([length(mpll), length(hpll),length(bpll_0)]);
  if (backup_n > 1)
    future_offset = min([length(mpll), length(hpll),length(bpll_0),length(bpll_1)]);
  elseif (backup_n > 2)
    future_offset = min([length(mpll), length(hpll),length(bpll_0),length(bpll_1),length(bpll_2)]);
  elseif (backup_n > 3)
    future_offset = min([length(mpll), length(hpll),length(bpll_0),length(bpll_1),length(bpll_2),length(bpll_3)]);
  end   
end

if(tmp_len - switchover < future_offset)
   mpll = [mpll(:,:);zeros(switchover+future_offset-tmp_len,size(mpll,2))];
elseif(tmp_len - switchover > future_offset)
   mpll = mpll(1:(switchover+future_offset),:);
end
disp('size mpll');
size(mpll)
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
%  bpll = [bpll_0;bpll_1;bpll_2;bpll_3];
start = switchover - 4000;
finish= switchover + 1000;
% fig 1
draw3(mpll, bpll, hpll, switchover, start, finish, option, backup_n);
start = switchover - 500;
finish= switchover + 300;
% fig 2
draw3(mpll, bpll, hpll, switchover, start, finish, option, backup_n);
start = switchover - 1000;
finish= switchover - 1;
% fig 13
draw3(mpll, bpll, hpll, switchover, start, finish, option, backup_n);
start = 1;
finish= size(mpll,1)
% fig 4
draw3(mpll, bpll, hpll, switchover, start, finish, option, backup_n);

if (backup_n > 0)
  start = switchover - 4000;
  finish= switchover + 1000;
  draw2(mpll, bpll_0, hpll, switchover, start, finish, option);
  start = switchover - 500;
  finish= switchover + 300;
  draw2(mpll, bpll_0, hpll, switchover, start, finish, option);
  start = switchover - 1000;
  finish= switchover - 1;
  draw2(mpll, bpll_0, hpll, switchover, start, finish, option);
end
if (backup_n > 1)
  start = switchover - 4000;
  finish= switchover + 1000;
  draw2(mpll, bpll_1, hpll, switchover, start, finish, option);
  start = switchover - 500;
  finish= switchover + 300;
  draw2(mpll, bpll_1, hpll, switchover, start, finish, option);
  start = switchover - 1000;
  finish= switchover - 1;
  draw2(mpll, bpll_1, hpll, switchover, start, finish, option);
end
if (backup_n > 2)
  start = switchover - 4000;
  finish= switchover + 1000;
  draw2(mpll, bpll_2, hpll, switchover, start, finish, option);
  start = switchover - 500;
  finish= switchover + 300;
  draw2(mpll, bpll_2, hpll, switchover, start, finish, option);
  start = switchover - 1000;
  finish= switchover - 1;
  draw2(mpll, bpll_2, hpll, switchover, start, finish, option);
end
if (backup_n > 3)
  start = switchover - 4000;
  finish= switchover + 1000;
  draw2(mpll, bpll_3, hpll, switchover, start, finish, option);
  start = switchover - 500;
  finish= switchover + 300;
  draw2(mpll, bpll_3, hpll, switchover, start, finish, option);
  start = switchover - 1000;
  finish= switchover - 1;
  draw2(mpll, bpll_3, hpll, switchover, start, finish, option);
end

return

