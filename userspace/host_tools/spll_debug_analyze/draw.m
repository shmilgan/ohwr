% Maciej Lipinski / CERN / 2014-10-22
% 
% scripts to analyzer debugging messages from the SoftPLL of the switch
% 
function draw(mERR, bERR, hERR, mY, anySO, switchover, start, finish)

figure
subplot(4,1,1)
  hold on; n =1; col ='b';
  plot(start:finish,mERR(start:finish,n),col,start:finish,anySO(start:finish)*max(mERR(start:finish,n)),'r' );
  n =2; col ='g';
  plot(start:finish,mERR(start:finish,n),col,start:finish,anySO(start:finish)*max(mERR(start:finish,n)),'r' );
  n =3; col ='r';
  plot(start:finish,mERR(start:finish,n),col,start:finish,anySO(start:finish)*max(mERR(start:finish,n)),'r' );
  n =4; col ='k';
  plot(start:finish,mERR(start:finish,n),col,start:finish,anySO(start:finish)*max(mERR(start:finish,n)),'r' );
  n =5; col ='c';
  plot(start:finish,mERR(start:finish,n),col,start:finish,anySO(start:finish)*max(mERR(start:finish,n)),'r' );
  n =6; col ='m';
  plot(start:finish,mERR(start:finish,n),col,start:finish,anySO(start:finish)*max(mERR(start:finish,n)),'r' );
  title('mPLL');
  legend('disconnect from SFP (b)','disconnect from SFP (g)','disconnect from SFP (r)','disconnect from SFP (k) ','disconnect LC-LC (c)','disconnect from SFP(long fiber) (m)');
subplot(4,1,2)
  hold on; n =1; col ='b';
  plot(start:finish,bERR(start:finish,n),col,start:finish,anySO(start:finish)*max(bERR(start:finish,n)),'r' );
  n =2; col ='g';
  plot(start:finish,bERR(start:finish,n),col,start:finish,anySO(start:finish)*max(bERR(start:finish,n)),'r' );
  n =3; col ='r';
  plot(start:finish,bERR(start:finish,n),col,start:finish,anySO(start:finish)*max(bERR(start:finish,n)),'r' );
  n =4; col ='k';
  plot(start:finish,bERR(start:finish,n),col,start:finish,anySO(start:finish)*max(bERR(start:finish,n)),'r' );
  n =5; col ='c';
  plot(start:finish,bERR(start:finish,n),col,start:finish,anySO(start:finish)*max(bERR(start:finish,n)),'r' );
  n =6; col ='m';
  plot(start:finish,bERR(start:finish,n),col,start:finish,anySO(start:finish)*max(bERR(start:finish,n)),'r' );
  title('bPLL');
  legend('disconnect from SFP (b)','disconnect from SFP (g)','disconnect from SFP (r)','disconnect from SFP (k) ','disconnect LC-LC (c)','disconnect from SFP(long fiber) (m)');
subplot(4,1,3)
  hold on; n =1; col ='b';
  plot(start:finish,hERR(start:finish,n),col,start:finish,anySO(start:finish)*max(hERR(start:finish,n)),'r' );
  n =2; col ='g';
  plot(start:finish,hERR(start:finish,n),col,start:finish,anySO(start:finish)*max(hERR(start:finish,n)),'r' );
  n =3; col ='r';
  plot(start:finish,hERR(start:finish,n),col,start:finish,anySO(start:finish)*max(hERR(start:finish,n)),'r' );
  n =4; col ='k';
  plot(start:finish,hERR(start:finish,n),col,start:finish,anySO(start:finish)*max(hERR(start:finish,n)),'r' );
  n =5; col ='c';
  plot(start:finish,hERR(start:finish,n),col,start:finish,anySO(start:finish)*max(hERR(start:finish,n)),'r' );
  n =6; col ='m';
  plot(start:finish,hERR(start:finish,n),col,start:finish,anySO(start:finish)*max(hERR(start:finish,n)),'r' );
  title('hPLL');
  legend('disconnect from SFP (b)','disconnect from SFP (g)','disconnect from SFP (r)','disconnect from SFP (k) ','disconnect LC-LC (c)','disconnect from SFP(long fiber) (m)');
subplot(4,1,4)
  hold on; n =1; col ='b';
  plot(start:finish,mY(start:finish,n),col,switchover,max(mY(start:finish,n)),'*r' );
  n =2; col ='g';
  plot(start:finish,mY(start:finish,n),col,switchover,max(mY(start:finish,n)),'*r' );
  n =3; col ='r';
  plot(start:finish,mY(start:finish,n),col,switchover,max(mY(start:finish,n)),'*r' );
  n =4; col ='k';
  plot(start:finish,mY(start:finish,n),col,switchover,max(mY(start:finish,n)),'*r' );
  n =5; col ='c';
  plot(start:finish,mY(start:finish,n),col,switchover,max(mY(start:finish,n)),'*r' );
  n =6; col ='m';
  plot(start:finish,mY(start:finish,n),col,switchover,max(mY(start:finish,n)),'*r' );
  title('Y');
  legend('disconnect from SFP (b)','disconnect from SFP (g)','disconnect from SFP (r)','disconnect from SFP (k) ','disconnect LC-LC (c)','disconnect from SFP(long fiber) (m)');

return