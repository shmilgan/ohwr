% Maciej Lipinski / CERN / 2014-10-22
% 
% scripts to analyzer debugging messages from the SoftPLL of the switch
% 
% options:
% 
% 
function drawTune(mpll, hpll, start, finish, option)

unitScale = (1/((62.5-(62.5*((2^14)/(1+2^14))))*10^6)); % [s]
%  Xaxis     = (start:finish)*unitScale;
Xaxis     = (1:length(mpll))*unitScale;
to_ps     = 16000/(2^14); 

figure
subplot(4,1,1)
hold on
if(option~=1)
  plot(Xaxis,mpll(:,5)*to_ps,'k'); % mpll error
  legend('err','Location','northwest');

  title('mPLL');
  xlabel('time [s]');
  ylabel('phase [ps]');
  axis tight
end



clr=['r','g','b','c']

y_max = max(hpll(:,2));
y_avg = mean(hpll(:,2));
err_max=max(hpll(:,3));
err_avg=mean(hpll(:,3));

y_event   = (~hpll(:,6))*y_avg   + (y_avg+10)*hpll(:,9);
err_event = (~hpll(:,6))*err_avg + err_max*hpll(:,9);
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
subplot(4,1,2)
hold on
%  plot(Xaxis,hpll(:,3)*to_ps,'k'); %hpll error
%  plot(Xaxis,err_event*to_ps,'r'); %hpll holdonver

plot(Xaxis(start:finish),hpll(start:finish,5),'k');
legend('lock_cnt','Location','northwest');
title('lock cont');
xlabel('time [s]');
ylabel('phase [ps]');
axis tight
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


subplot(4,1,3)
hold on
if(option~=1)
  plot(Xaxis(start:finish),mpll(start:finish,5)*to_ps,'k');
  legend('err','Location','northwest');

  title('mPLL');
  xlabel('time [s]');
  ylabel('phase [ps]');
  axis tight
end

plot(Xaxis(start:finish),hpll(start:finish,2)*to_ps,'k');

  title('hPLL');
  xlabel('time [s]');
  ylabel('Y [ps]');
  axis tight

clr=['r','g','b','c']

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
subplot(4,1,4)
hold on
plot(Xaxis(start:finish),hpll(start:finish,3)*to_ps,'k');
plot(Xaxis(start:finish),err_event(start:finish)*to_ps,'r');
title('hPLLs');
legend('err','delock','Location','northwest');
xlabel('time [s]');
ylabel('phase [ps]');
axis tight
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%









figure
subplot(4,1,1)
hold on
if(option~=1)
  plot(Xaxis,mpll(:,2)*to_ps,'k');
  legend('Y','Location','northwest');

  title('mPLL');
  xlabel('time [s]');
  ylabel('DACout');
  axis tight
end
clr=['r','g','b','c']

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
subplot(4,1,2)
hold on
plot(Xaxis,hpll(:,2)*to_ps,'k');
%  plot(Xaxis,y_event*to_ps,'r');

legend('Y','delock','Location','northwest');
title('hPLLs');
xlabel('time [s]');
ylabel('DACout');
axis tight
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


subplot(4,1,3)
hold on
if(option~=1)
  plot(Xaxis(start:finish),mpll(start:finish,2)*to_ps,'k');
  legend('Y','Location','northwest');

  title('mPLL');
  xlabel('time [s]');
  ylabel('DACout');
  axis tight
end
clr=['r','g','b','c']

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
subplot(4,1,4)
hold on
plot(Xaxis(start:finish),hpll(start:finish,2)*to_ps,'k');
plot(Xaxis(start:finish), y_event(start:finish)*to_ps,'r');

legend('Y','delock','Location','northwest');
title('b=hPLLs');
xlabel('time [s]');
ylabel('DACout');
axis tight

return