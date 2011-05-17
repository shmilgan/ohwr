
bin/mch_wd_dummy-flash.elf:     file format elf32-littlearm

Disassembly of section .fixed:

00100000 <_sfixed>:
  100000:	e59f006c 	ldr	r0, [pc, #108]	; 100074 <.fixed+0x74>
  100004:	e59ff06c 	ldr	pc, [pc, #108]	; 100078 <.fixed+0x78>
  100008:	e59f406c 	ldr	r4, [pc, #108]	; 10007c <.fixed+0x7c>
  10000c:	e1a0d004 	mov	sp, r4
  100010:	e59f0068 	ldr	r0, [pc, #104]	; 100080 <.fixed+0x80>
  100014:	e1a0e00f 	mov	lr, pc
  100018:	e12fff10 	bx	r0
  10001c:	e59f0060 	ldr	r0, [pc, #96]	; 100084 <.fixed+0x84>
  100020:	e59f1060 	ldr	r1, [pc, #96]	; 100088 <.fixed+0x88>
  100024:	e59f2060 	ldr	r2, [pc, #96]	; 10008c <.fixed+0x8c>
  100028:	e1510002 	cmp	r1, r2
  10002c:	34903004 	ldrcc	r3, [r0], #4
  100030:	34813004 	strcc	r3, [r1], #4
  100034:	3afffffb 	bcc	100028 <_sfixed+0x28>
  100038:	e59f0050 	ldr	r0, [pc, #80]	; 100090 <.fixed+0x90>
  10003c:	e59f1050 	ldr	r1, [pc, #80]	; 100094 <.fixed+0x94>
  100040:	e3a02000 	mov	r2, #0	; 0x0
  100044:	e1500001 	cmp	r0, r1
  100048:	34802004 	strcc	r2, [r0], #4
  10004c:	3afffffc 	bcc	100044 <_sfixed+0x44>
  100050:	e321f0d2 	msr	CPSR_c, #210	; 0xd2
  100054:	e1a0d004 	mov	sp, r4
  100058:	e2444060 	sub	r4, r4, #96	; 0x60
  10005c:	e321f053 	msr	CPSR_c, #83	; 0x53
  100060:	e1a0d004 	mov	sp, r4
  100064:	e59f002c 	ldr	r0, [pc, #44]	; 100098 <.fixed+0x98>
  100068:	e1a0e00f 	mov	lr, pc
  10006c:	e12fff10 	bx	r0
  100070:	eafffffe 	b	100070 <_sfixed+0x70>
  100074:	00200000 	eoreq	r0, r0, r0
  100078:	00100008 	andeqs	r0, r0, r8
  10007c:	00220000 	eoreq	r0, r2, r0
  100080:	00100c80 	andeqs	r0, r0, r0, lsl #25
  100084:	00101030 	andeqs	r1, r0, r0, lsr r0
  100088:	00200000 	eoreq	r0, r0, r0
  10008c:	002003f0 	streqd	r0, [r0], -r0
  100090:	002003f0 	streqd	r0, [r0], -r0
  100094:	002003f0 	streqd	r0, [r0], -r0
  100098:	00100178 	andeqs	r0, r0, r8, ror r1
  10009c:	e1a00000 	nop			(mov r0,r0)

001000a0 <delay>:
  1000a0:	e24dd004 	sub	sp, sp, #4	; 0x4
  1000a4:	e2500001 	subs	r0, r0, #1	; 0x1
  1000a8:	3a00000b 	bcc	1000dc <delay+0x3c>
  1000ac:	e3a03000 	mov	r3, #0	; 0x0
  1000b0:	e58d3000 	str	r3, [sp]
  1000b4:	e59d3000 	ldr	r3, [sp]
  1000b8:	e59f2024 	ldr	r2, [pc, #36]	; 1000e4 <.fixed+0xe4>
  1000bc:	e1530002 	cmp	r3, r2
  1000c0:	cafffff7 	bgt	1000a4 <delay+0x4>
  1000c4:	e1a00000 	nop			(mov r0,r0)
  1000c8:	e59d3000 	ldr	r3, [sp]
  1000cc:	e2833001 	add	r3, r3, #1	; 0x1
  1000d0:	e58d3000 	str	r3, [sp]
  1000d4:	e59d3000 	ldr	r3, [sp]
  1000d8:	eafffff7 	b	1000bc <delay+0x1c>
  1000dc:	e28dd004 	add	sp, sp, #4	; 0x4
  1000e0:	e1a0f00e 	mov	pc, lr
  1000e4:	0000270f 	andeq	r2, r0, pc, lsl #14

001000e8 <io_init>:
  1000e8:	e1a0c00d 	mov	ip, sp
  1000ec:	e92dd810 	stmdb	sp!, {r4, fp, ip, lr, pc}
  1000f0:	e3a01001 	mov	r1, #1	; 0x1
  1000f4:	e24cb004 	sub	fp, ip, #4	; 0x4
  1000f8:	e59f405c 	ldr	r4, [pc, #92]	; 10015c <.fixed+0x15c>
  1000fc:	e59f005c 	ldr	r0, [pc, #92]	; 100160 <.fixed+0x160>
  100100:	e1a0e00f 	mov	lr, pc
  100104:	e1a0f004 	mov	pc, r4
  100108:	e3a01001 	mov	r1, #1	; 0x1
  10010c:	e59f0050 	ldr	r0, [pc, #80]	; 100164 <.fixed+0x164>
  100110:	e1a0e00f 	mov	lr, pc
  100114:	e1a0f004 	mov	pc, r4
  100118:	e3a01001 	mov	r1, #1	; 0x1
  10011c:	e59f0044 	ldr	r0, [pc, #68]	; 100168 <.fixed+0x168>
  100120:	e1a0e00f 	mov	lr, pc
  100124:	e1a0f004 	mov	pc, r4
  100128:	e3a01001 	mov	r1, #1	; 0x1
  10012c:	e59f0038 	ldr	r0, [pc, #56]	; 10016c <.fixed+0x16c>
  100130:	e1a0e00f 	mov	lr, pc
  100134:	e1a0f004 	mov	pc, r4
  100138:	e3a01001 	mov	r1, #1	; 0x1
  10013c:	e59f002c 	ldr	r0, [pc, #44]	; 100170 <.fixed+0x170>
  100140:	e1a0e00f 	mov	lr, pc
  100144:	e1a0f004 	mov	pc, r4
  100148:	e3a01001 	mov	r1, #1	; 0x1
  10014c:	e59f0020 	ldr	r0, [pc, #32]	; 100174 <.fixed+0x174>
  100150:	e1a0e00f 	mov	lr, pc
  100154:	e1a0f004 	mov	pc, r4
  100158:	e89da810 	ldmia	sp, {r4, fp, sp, pc}
  10015c:	00100a9c 	muleqs	r0, ip, sl
  100160:	00100fb0 	ldreqh	r0, [r0], -r0
  100164:	00100fa4 	andeqs	r0, r0, r4, lsr #31
  100168:	00100fc8 	andeqs	r0, r0, r8, asr #31
  10016c:	00100fd4 	ldreqsb	r0, [r0], -r4
  100170:	00100fbc 	ldreqh	r0, [r0], -ip
  100174:	00100fe0 	andeqs	r0, r0, r0, ror #31

00100178 <main>:
  100178:	e1a0c00d 	mov	ip, sp
  10017c:	e92dd870 	stmdb	sp!, {r4, r5, r6, fp, ip, lr, pc}
  100180:	e24cb004 	sub	fp, ip, #4	; 0x4
  100184:	e24dd00c 	sub	sp, sp, #12	; 0xc
  100188:	e59f3178 	ldr	r3, [pc, #376]	; 100308 <.fixed+0x308>
  10018c:	e1a0e00f 	mov	lr, pc
  100190:	e1a0f003 	mov	pc, r3
  100194:	e59f3170 	ldr	r3, [pc, #368]	; 10030c <.fixed+0x30c>
  100198:	e59f0170 	ldr	r0, [pc, #368]	; 100310 <.fixed+0x310>
  10019c:	e1a0e00f 	mov	lr, pc
  1001a0:	e1a0f003 	mov	pc, r3
  1001a4:	e59f3168 	ldr	r3, [pc, #360]	; 100314 <.fixed+0x314>
  1001a8:	e31000ff 	tst	r0, #255	; 0xff
  1001ac:	e8930007 	ldmia	r3, {r0, r1, r2}
  1001b0:	e24b3024 	sub	r3, fp, #36	; 0x24
  1001b4:	e8830007 	stmia	r3, {r0, r1, r2}
  1001b8:	e1a00003 	mov	r0, r3
  1001bc:	e3a01001 	mov	r1, #1	; 0x1
  1001c0:	e59f3150 	ldr	r3, [pc, #336]	; 100318 <.fixed+0x318>
  1001c4:	13a04000 	movne	r4, #0	; 0x0
  1001c8:	03a04001 	moveq	r4, #1	; 0x1
  1001cc:	e1a0e00f 	mov	lr, pc
  1001d0:	e1a0f003 	mov	pc, r3
  1001d4:	e3a00b02 	mov	r0, #2048	; 0x800
  1001d8:	e59f113c 	ldr	r1, [pc, #316]	; 10031c <.fixed+0x31c>
  1001dc:	e59f213c 	ldr	r2, [pc, #316]	; 100320 <.fixed+0x320>
  1001e0:	e59f313c 	ldr	r3, [pc, #316]	; 100324 <.fixed+0x324>
  1001e4:	e1a0e00f 	mov	lr, pc
  1001e8:	e1a0f003 	mov	pc, r3
  1001ec:	e1a01004 	mov	r1, r4
  1001f0:	e59f0130 	ldr	r0, [pc, #304]	; 100328 <.fixed+0x328>
  1001f4:	e59f3130 	ldr	r3, [pc, #304]	; 10032c <.fixed+0x32c>
  1001f8:	e1a0e00f 	mov	lr, pc
  1001fc:	e1a0f003 	mov	pc, r3
  100200:	e3540000 	cmp	r4, #0	; 0x0
  100204:	e59f5124 	ldr	r5, [pc, #292]	; 100330 <.fixed+0x330>
  100208:	e59f4124 	ldr	r4, [pc, #292]	; 100334 <.fixed+0x334>
  10020c:	e59f6124 	ldr	r6, [pc, #292]	; 100338 <.fixed+0x338>
  100210:	0a000012 	beq	100260 <main+0xe8>
  100214:	e59f0120 	ldr	r0, [pc, #288]	; 10033c <.fixed+0x33c>
  100218:	e1a0e00f 	mov	lr, pc
  10021c:	e1a0f004 	mov	pc, r4
  100220:	e59f0118 	ldr	r0, [pc, #280]	; 100340 <.fixed+0x340>
  100224:	e1a0e00f 	mov	lr, pc
  100228:	e1a0f004 	mov	pc, r4
  10022c:	e3a00014 	mov	r0, #20	; 0x14
  100230:	e1a0e00f 	mov	lr, pc
  100234:	e1a0f005 	mov	pc, r5
  100238:	e59f0104 	ldr	r0, [pc, #260]	; 100344 <.fixed+0x344>
  10023c:	e1a0e00f 	mov	lr, pc
  100240:	e1a0f004 	mov	pc, r4
  100244:	e3a000c8 	mov	r0, #200	; 0xc8
  100248:	e1a0e00f 	mov	lr, pc
  10024c:	e1a0f005 	mov	pc, r5
  100250:	e59f00e4 	ldr	r0, [pc, #228]	; 10033c <.fixed+0x33c>
  100254:	e1a0e00f 	mov	lr, pc
  100258:	e1a0f006 	mov	pc, r6
  10025c:	ea00000b 	b	100290 <main+0x118>
  100260:	e59f00d4 	ldr	r0, [pc, #212]	; 10033c <.fixed+0x33c>
  100264:	e1a0e00f 	mov	lr, pc
  100268:	e1a0f006 	mov	pc, r6
  10026c:	e59f00cc 	ldr	r0, [pc, #204]	; 100340 <.fixed+0x340>
  100270:	e1a0e00f 	mov	lr, pc
  100274:	e1a0f004 	mov	pc, r4
  100278:	e3a00014 	mov	r0, #20	; 0x14
  10027c:	e1a0e00f 	mov	lr, pc
  100280:	e1a0f005 	mov	pc, r5
  100284:	e59f00b8 	ldr	r0, [pc, #184]	; 100344 <.fixed+0x344>
  100288:	e1a0e00f 	mov	lr, pc
  10028c:	e1a0f004 	mov	pc, r4
  100290:	e59f30b0 	ldr	r3, [pc, #176]	; 100348 <.fixed+0x348>
  100294:	e3a0002c 	mov	r0, #44	; 0x2c
  100298:	e1a0e00f 	mov	lr, pc
  10029c:	e1a0f003 	mov	pc, r3
  1002a0:	e59f608c 	ldr	r6, [pc, #140]	; 100334 <.fixed+0x334>
  1002a4:	e59f00a0 	ldr	r0, [pc, #160]	; 10034c <.fixed+0x34c>
  1002a8:	e59f4080 	ldr	r4, [pc, #128]	; 100330 <.fixed+0x330>
  1002ac:	e1a0e00f 	mov	lr, pc
  1002b0:	e1a0f006 	mov	pc, r6
  1002b4:	e3a0000a 	mov	r0, #10	; 0xa
  1002b8:	e1a0e00f 	mov	lr, pc
  1002bc:	e1a0f004 	mov	pc, r4
  1002c0:	e59f5070 	ldr	r5, [pc, #112]	; 100338 <.fixed+0x338>
  1002c4:	e59f0080 	ldr	r0, [pc, #128]	; 10034c <.fixed+0x34c>
  1002c8:	e1a0e00f 	mov	lr, pc
  1002cc:	e1a0f005 	mov	pc, r5
  1002d0:	e3a0000a 	mov	r0, #10	; 0xa
  1002d4:	e1a0e00f 	mov	lr, pc
  1002d8:	e1a0f004 	mov	pc, r4
  1002dc:	e59f006c 	ldr	r0, [pc, #108]	; 100350 <.fixed+0x350>
  1002e0:	e1a0e00f 	mov	lr, pc
  1002e4:	e1a0f006 	mov	pc, r6
  1002e8:	e3a0000a 	mov	r0, #10	; 0xa
  1002ec:	e1a0e00f 	mov	lr, pc
  1002f0:	e1a0f004 	mov	pc, r4
  1002f4:	e59f0054 	ldr	r0, [pc, #84]	; 100350 <.fixed+0x350>
  1002f8:	e1a0e00f 	mov	lr, pc
  1002fc:	e1a0f005 	mov	pc, r5
  100300:	e3a0000a 	mov	r0, #10	; 0xa
  100304:	eaffffdf 	b	100288 <main+0x110>
  100308:	001000e8 	andeqs	r0, r0, r8, ror #1
  10030c:	00100be4 	andeqs	r0, r0, r4, ror #23
  100310:	00100fe0 	andeqs	r0, r0, r0, ror #31
  100314:	00100fec 	andeqs	r0, r0, ip, ror #31
  100318:	00100a9c 	muleqs	r0, ip, sl
  10031c:	0001c200 	andeq	ip, r1, r0, lsl #4
  100320:	02dc6c00 	sbceqs	r6, ip, #0	; 0x0
  100324:	00100948 	andeqs	r0, r0, r8, asr #18
  100328:	00100ff8 	ldreqsh	r0, [r0], -r8
  10032c:	0010091c 	andeqs	r0, r0, ip, lsl r9
  100330:	001000a0 	andeqs	r0, r0, r0, lsr #1
  100334:	00100bcc 	andeqs	r0, r0, ip, asr #23
  100338:	00100bd8 	ldreqsb	r0, [r0], -r8
  10033c:	00100fd4 	ldreqsb	r0, [r0], -r4
  100340:	00100fc8 	andeqs	r0, r0, r8, asr #31
  100344:	00100fbc 	ldreqh	r0, [r0], -ip
  100348:	00100a88 	andeqs	r0, r0, r8, lsl #21
  10034c:	00100fb0 	ldreqh	r0, [r0], -r0
  100350:	00100fa4 	andeqs	r0, r0, r4, lsr #31

00100354 <PutChar>:
  100354:	e5c01000 	strb	r1, [r0]
  100358:	e3a00001 	mov	r0, #1	; 0x1
  10035c:	e1a0f00e 	mov	pc, lr

00100360 <PutString>:
  100360:	e5d13000 	ldrb	r3, [r1]
  100364:	e3530000 	cmp	r3, #0	; 0x0
  100368:	e1a02000 	mov	r2, r0
  10036c:	e3a00000 	mov	r0, #0	; 0x0
  100370:	01a0f00e 	moveq	pc, lr
  100374:	e4c23001 	strb	r3, [r2], #1
  100378:	e5f13001 	ldrb	r3, [r1, #1]!
  10037c:	e3530000 	cmp	r3, #0	; 0x0
  100380:	e2800001 	add	r0, r0, #1	; 0x1
  100384:	1afffffa 	bne	100374 <PutString+0x14>
  100388:	e1a0f00e 	mov	pc, lr

0010038c <PutUnsignedInt>:
  10038c:	e1a0c00d 	mov	ip, sp
  100390:	e92dd9f0 	stmdb	sp!, {r4, r5, r6, r7, r8, fp, ip, lr, pc}
  100394:	e1a08003 	mov	r8, r3
  100398:	e1a03001 	mov	r3, r1
  10039c:	e24cb004 	sub	fp, ip, #4	; 0x4
  1003a0:	e1a05000 	mov	r5, r0
  1003a4:	e3a0100a 	mov	r1, #10	; 0xa
  1003a8:	e1a00008 	mov	r0, r8
  1003ac:	e20370ff 	and	r7, r3, #255	; 0xff
  1003b0:	e59f30a0 	ldr	r3, [pc, #160]	; 100458 <.fixed+0x458>
  1003b4:	e2424001 	sub	r4, r2, #1	; 0x1
  1003b8:	e1a0e00f 	mov	lr, pc
  1003bc:	e1a0f003 	mov	pc, r3
  1003c0:	e3500000 	cmp	r0, #0	; 0x0
  1003c4:	e3a06000 	mov	r6, #0	; 0x0
  1003c8:	0a000009 	beq	1003f4 <PutUnsignedInt+0x68>
  1003cc:	e1a03000 	mov	r3, r0
  1003d0:	e1a01007 	mov	r1, r7
  1003d4:	e1a00005 	mov	r0, r5
  1003d8:	e1a02004 	mov	r2, r4
  1003dc:	e59fc078 	ldr	ip, [pc, #120]	; 10045c <.fixed+0x45c>
  1003e0:	e1a0e00f 	mov	lr, pc
  1003e4:	e1a0f00c 	mov	pc, ip
  1003e8:	e1a06000 	mov	r6, r0
  1003ec:	e0855000 	add	r5, r5, r0
  1003f0:	ea00000b 	b	100424 <PutUnsignedInt+0x98>
  1003f4:	e3540000 	cmp	r4, #0	; 0x0
  1003f8:	da000009 	ble	100424 <PutUnsignedInt+0x98>
  1003fc:	e1a00005 	mov	r0, r5
  100400:	e1a01007 	mov	r1, r7
  100404:	e59f3054 	ldr	r3, [pc, #84]	; 100460 <.fixed+0x460>
  100408:	e1a0e00f 	mov	lr, pc
  10040c:	e1a0f003 	mov	pc, r3
  100410:	e2444001 	sub	r4, r4, #1	; 0x1
  100414:	e3540000 	cmp	r4, #0	; 0x0
  100418:	e2855001 	add	r5, r5, #1	; 0x1
  10041c:	e2866001 	add	r6, r6, #1	; 0x1
  100420:	eafffff4 	b	1003f8 <PutUnsignedInt+0x6c>
  100424:	e1a00008 	mov	r0, r8
  100428:	e3a0100a 	mov	r1, #10	; 0xa
  10042c:	e59f3030 	ldr	r3, [pc, #48]	; 100464 <.fixed+0x464>
  100430:	e1a0e00f 	mov	lr, pc
  100434:	e1a0f003 	mov	pc, r3
  100438:	e2801030 	add	r1, r0, #48	; 0x30
  10043c:	e20110ff 	and	r1, r1, #255	; 0xff
  100440:	e1a00005 	mov	r0, r5
  100444:	e59f3014 	ldr	r3, [pc, #20]	; 100460 <.fixed+0x460>
  100448:	e1a0e00f 	mov	lr, pc
  10044c:	e1a0f003 	mov	pc, r3
  100450:	e0860000 	add	r0, r6, r0
  100454:	e89da9f0 	ldmia	sp, {r4, r5, r6, r7, r8, fp, sp, pc}
  100458:	00100ddc 	ldreqsb	r0, [r0], -ip
  10045c:	0010038c 	andeqs	r0, r0, ip, lsl #7
  100460:	00100354 	andeqs	r0, r0, r4, asr r3
  100464:	00100ed4 	ldreqsb	r0, [r0], -r4

00100468 <PutSignedInt>:
  100468:	e1a0c00d 	mov	ip, sp
  10046c:	e92dddf0 	stmdb	sp!, {r4, r5, r6, r7, r8, sl, fp, ip, lr, pc}
  100470:	e023afc3 	eor	sl, r3, r3, asr #31
  100474:	e04aafc3 	sub	sl, sl, r3, asr #31
  100478:	e1a06003 	mov	r6, r3
  10047c:	e1a03001 	mov	r3, r1
  100480:	e24cb004 	sub	fp, ip, #4	; 0x4
  100484:	e1a05000 	mov	r5, r0
  100488:	e3a0100a 	mov	r1, #10	; 0xa
  10048c:	e1a0000a 	mov	r0, sl
  100490:	e20380ff 	and	r8, r3, #255	; 0xff
  100494:	e59f30dc 	ldr	r3, [pc, #220]	; 100578 <.fixed+0x578>
  100498:	e2424001 	sub	r4, r2, #1	; 0x1
  10049c:	e1a0e00f 	mov	lr, pc
  1004a0:	e1a0f003 	mov	pc, r3
  1004a4:	e3500000 	cmp	r0, #0	; 0x0
  1004a8:	e3a07000 	mov	r7, #0	; 0x0
  1004ac:	0a00000d 	beq	1004e8 <PutSignedInt+0x80>
  1004b0:	e1560007 	cmp	r6, r7
  1004b4:	b2603000 	rsblt	r3, r0, #0	; 0x0
  1004b8:	a1a03000 	movge	r3, r0
  1004bc:	e59fc0b8 	ldr	ip, [pc, #184]	; 10057c <.fixed+0x57c>
  1004c0:	e1a00005 	mov	r0, r5
  1004c4:	b1a01008 	movlt	r1, r8
  1004c8:	b1a02004 	movlt	r2, r4
  1004cc:	a1a01008 	movge	r1, r8
  1004d0:	a1a02004 	movge	r2, r4
  1004d4:	e1a0e00f 	mov	lr, pc
  1004d8:	e1a0f00c 	mov	pc, ip
  1004dc:	e1a07000 	mov	r7, r0
  1004e0:	e0855000 	add	r5, r5, r0
  1004e4:	ea000016 	b	100544 <PutSignedInt+0xdc>
  1004e8:	e3560000 	cmp	r6, #0	; 0x0
  1004ec:	b2444001 	sublt	r4, r4, #1	; 0x1
  1004f0:	e3540000 	cmp	r4, #0	; 0x0
  1004f4:	da000009 	ble	100520 <PutSignedInt+0xb8>
  1004f8:	e1a00005 	mov	r0, r5
  1004fc:	e1a01008 	mov	r1, r8
  100500:	e59f3078 	ldr	r3, [pc, #120]	; 100580 <.fixed+0x580>
  100504:	e1a0e00f 	mov	lr, pc
  100508:	e1a0f003 	mov	pc, r3
  10050c:	e2444001 	sub	r4, r4, #1	; 0x1
  100510:	e3540000 	cmp	r4, #0	; 0x0
  100514:	e2855001 	add	r5, r5, #1	; 0x1
  100518:	e2877001 	add	r7, r7, #1	; 0x1
  10051c:	eafffff4 	b	1004f4 <PutSignedInt+0x8c>
  100520:	e3560000 	cmp	r6, #0	; 0x0
  100524:	aa000006 	bge	100544 <PutSignedInt+0xdc>
  100528:	e1a00005 	mov	r0, r5
  10052c:	e3a0102d 	mov	r1, #45	; 0x2d
  100530:	e59f3048 	ldr	r3, [pc, #72]	; 100580 <.fixed+0x580>
  100534:	e1a0e00f 	mov	lr, pc
  100538:	e1a0f003 	mov	pc, r3
  10053c:	e2855001 	add	r5, r5, #1	; 0x1
  100540:	e0877000 	add	r7, r7, r0
  100544:	e1a0000a 	mov	r0, sl
  100548:	e3a0100a 	mov	r1, #10	; 0xa
  10054c:	e59f3030 	ldr	r3, [pc, #48]	; 100584 <.fixed+0x584>
  100550:	e1a0e00f 	mov	lr, pc
  100554:	e1a0f003 	mov	pc, r3
  100558:	e2801030 	add	r1, r0, #48	; 0x30
  10055c:	e20110ff 	and	r1, r1, #255	; 0xff
  100560:	e1a00005 	mov	r0, r5
  100564:	e59f3014 	ldr	r3, [pc, #20]	; 100580 <.fixed+0x580>
  100568:	e1a0e00f 	mov	lr, pc
  10056c:	e1a0f003 	mov	pc, r3
  100570:	e0870000 	add	r0, r7, r0
  100574:	e89dadf0 	ldmia	sp, {r4, r5, r6, r7, r8, sl, fp, sp, pc}
  100578:	00100ddc 	ldreqsb	r0, [r0], -ip
  10057c:	00100468 	andeqs	r0, r0, r8, ror #8
  100580:	00100354 	andeqs	r0, r0, r4, asr r3
  100584:	00100ed4 	ldreqsb	r0, [r0], -r4

00100588 <PutHexa>:
  100588:	e1a0c00d 	mov	ip, sp
  10058c:	e92dddf0 	stmdb	sp!, {r4, r5, r6, r7, r8, sl, fp, ip, lr, pc}
  100590:	e24cb004 	sub	fp, ip, #4	; 0x4
  100594:	e24dd004 	sub	sp, sp, #4	; 0x4
  100598:	e59b8004 	ldr	r8, [fp, #4]
  10059c:	e1b0c228 	movs	ip, r8, lsr #4
  1005a0:	e20160ff 	and	r6, r1, #255	; 0xff
  1005a4:	e203a0ff 	and	sl, r3, #255	; 0xff
  1005a8:	e2425001 	sub	r5, r2, #1	; 0x1
  1005ac:	e1a04000 	mov	r4, r0
  1005b0:	e3a07000 	mov	r7, #0	; 0x0
  1005b4:	0a000009 	beq	1005e0 <PutHexa+0x58>
  1005b8:	e58dc000 	str	ip, [sp]
  1005bc:	e1a01006 	mov	r1, r6
  1005c0:	e1a02005 	mov	r2, r5
  1005c4:	e1a0300a 	mov	r3, sl
  1005c8:	e59fc080 	ldr	ip, [pc, #128]	; 100650 <.fixed+0x650>
  1005cc:	e1a0e00f 	mov	lr, pc
  1005d0:	e1a0f00c 	mov	pc, ip
  1005d4:	e1a07000 	mov	r7, r0
  1005d8:	e0844000 	add	r4, r4, r0
  1005dc:	ea00000b 	b	100610 <PutHexa+0x88>
  1005e0:	e3550000 	cmp	r5, #0	; 0x0
  1005e4:	da000009 	ble	100610 <PutHexa+0x88>
  1005e8:	e1a00004 	mov	r0, r4
  1005ec:	e1a01006 	mov	r1, r6
  1005f0:	e59f305c 	ldr	r3, [pc, #92]	; 100654 <.fixed+0x654>
  1005f4:	e1a0e00f 	mov	lr, pc
  1005f8:	e1a0f003 	mov	pc, r3
  1005fc:	e2455001 	sub	r5, r5, #1	; 0x1
  100600:	e3550000 	cmp	r5, #0	; 0x0
  100604:	e2844001 	add	r4, r4, #1	; 0x1
  100608:	e2877001 	add	r7, r7, #1	; 0x1
  10060c:	eafffff4 	b	1005e4 <PutHexa+0x5c>
  100610:	e208100f 	and	r1, r8, #15	; 0xf
  100614:	e3510009 	cmp	r1, #9	; 0x9
  100618:	91a00004 	movls	r0, r4
  10061c:	92811030 	addls	r1, r1, #48	; 0x30
  100620:	959f302c 	ldrls	r3, [pc, #44]	; 100654 <.fixed+0x654>
  100624:	9a000005 	bls	100640 <PutHexa+0xb8>
  100628:	e35a0000 	cmp	sl, #0	; 0x0
  10062c:	e59f3020 	ldr	r3, [pc, #32]	; 100654 <.fixed+0x654>
  100630:	11a00004 	movne	r0, r4
  100634:	12811037 	addne	r1, r1, #55	; 0x37
  100638:	01a00004 	moveq	r0, r4
  10063c:	02811057 	addeq	r1, r1, #87	; 0x57
  100640:	e1a0e00f 	mov	lr, pc
  100644:	e1a0f003 	mov	pc, r3
  100648:	e2870001 	add	r0, r7, #1	; 0x1
  10064c:	e89dadf8 	ldmia	sp, {r3, r4, r5, r6, r7, r8, sl, fp, sp, pc}
  100650:	00100588 	andeqs	r0, r0, r8, lsl #11
  100654:	00100354 	andeqs	r0, r0, r4, asr r3

00100658 <vsnprintf>:
  100658:	e1a0c00d 	mov	ip, sp
  10065c:	e92dd9f0 	stmdb	sp!, {r4, r5, r6, r7, r8, fp, ip, lr, pc}
  100660:	e24cb004 	sub	fp, ip, #4	; 0x4
  100664:	e24dd004 	sub	sp, sp, #4	; 0x4
  100668:	e2506000 	subs	r6, r0, #0	; 0x0
  10066c:	e3a07000 	mov	r7, #0	; 0x0
  100670:	15c67000 	strneb	r7, [r6]
  100674:	e1a08001 	mov	r8, r1
  100678:	e1a05002 	mov	r5, r2
  10067c:	e1a04003 	mov	r4, r3
  100680:	e5d53000 	ldrb	r3, [r5]
  100684:	e20320ff 	and	r2, r3, #255	; 0xff
  100688:	e3520000 	cmp	r2, #0	; 0x0
  10068c:	11570008 	cmpne	r7, r8
  100690:	2a00005c 	bcs	100808 <vsnprintf+0x1b0>
  100694:	e3520025 	cmp	r2, #37	; 0x25
  100698:	14c63001 	strneb	r3, [r6], #1
  10069c:	12855001 	addne	r5, r5, #1	; 0x1
  1006a0:	1a000004 	bne	1006b8 <vsnprintf+0x60>
  1006a4:	e5d53001 	ldrb	r3, [r5, #1]
  1006a8:	e3530025 	cmp	r3, #37	; 0x25
  1006ac:	1a000003 	bne	1006c0 <vsnprintf+0x68>
  1006b0:	e4c62001 	strb	r2, [r6], #1
  1006b4:	e2855002 	add	r5, r5, #2	; 0x2
  1006b8:	e2877001 	add	r7, r7, #1	; 0x1
  1006bc:	eaffffef 	b	100680 <vsnprintf+0x28>
  1006c0:	e5f53001 	ldrb	r3, [r5, #1]!
  1006c4:	e3530030 	cmp	r3, #48	; 0x30
  1006c8:	02855001 	addeq	r5, r5, #1	; 0x1
  1006cc:	e5d50000 	ldrb	r0, [r5]
  1006d0:	e3a01020 	mov	r1, #32	; 0x20
  1006d4:	01a01003 	moveq	r1, r3
  1006d8:	e2403030 	sub	r3, r0, #48	; 0x30
  1006dc:	e3a02000 	mov	r2, #0	; 0x0
  1006e0:	e3530009 	cmp	r3, #9	; 0x9
  1006e4:	8a000007 	bhi	100708 <vsnprintf+0xb0>
  1006e8:	e0823102 	add	r3, r2, r2, lsl #2
  1006ec:	e0803083 	add	r3, r0, r3, lsl #1
  1006f0:	e5f50001 	ldrb	r0, [r5, #1]!
  1006f4:	e2433030 	sub	r3, r3, #48	; 0x30
  1006f8:	e2402030 	sub	r2, r0, #48	; 0x30
  1006fc:	e3520009 	cmp	r2, #9	; 0x9
  100700:	e20320ff 	and	r2, r3, #255	; 0xff
  100704:	eafffff6 	b	1006e4 <vsnprintf+0x8c>
  100708:	e0873002 	add	r3, r7, r2
  10070c:	e1530008 	cmp	r3, r8
  100710:	80673008 	rsbhi	r3, r7, r8
  100714:	820320ff 	andhi	r2, r3, #255	; 0xff
  100718:	e5d53000 	ldrb	r3, [r5]
  10071c:	e3530069 	cmp	r3, #105	; 0x69
  100720:	0a000012 	beq	100770 <vsnprintf+0x118>
  100724:	ca000008 	bgt	10074c <vsnprintf+0xf4>
  100728:	e3530063 	cmp	r3, #99	; 0x63
  10072c:	0a000029 	beq	1007d8 <vsnprintf+0x180>
  100730:	ca000002 	bgt	100740 <vsnprintf+0xe8>
  100734:	e3530058 	cmp	r3, #88	; 0x58
  100738:	0a00001a 	beq	1007a8 <vsnprintf+0x150>
  10073c:	ea00002f 	b	100800 <vsnprintf+0x1a8>
  100740:	e3530064 	cmp	r3, #100	; 0x64
  100744:	0a000009 	beq	100770 <vsnprintf+0x118>
  100748:	ea00002c 	b	100800 <vsnprintf+0x1a8>
  10074c:	e3530075 	cmp	r3, #117	; 0x75
  100750:	0a00000a 	beq	100780 <vsnprintf+0x128>
  100754:	ca000002 	bgt	100764 <vsnprintf+0x10c>
  100758:	e3530073 	cmp	r3, #115	; 0x73
  10075c:	0a000019 	beq	1007c8 <vsnprintf+0x170>
  100760:	ea000026 	b	100800 <vsnprintf+0x1a8>
  100764:	e3530078 	cmp	r3, #120	; 0x78
  100768:	0a00000a 	beq	100798 <vsnprintf+0x140>
  10076c:	ea000023 	b	100800 <vsnprintf+0x1a8>
  100770:	e5943000 	ldr	r3, [r4]
  100774:	e59fc0ac 	ldr	ip, [pc, #172]	; 100828 <.fixed+0x828>
  100778:	e1a00006 	mov	r0, r6
  10077c:	ea000002 	b	10078c <vsnprintf+0x134>
  100780:	e5943000 	ldr	r3, [r4]
  100784:	e59fc0a0 	ldr	ip, [pc, #160]	; 10082c <.fixed+0x82c>
  100788:	e1a00006 	mov	r0, r6
  10078c:	e1a0e00f 	mov	lr, pc
  100790:	e1a0f00c 	mov	pc, ip
  100794:	ea000014 	b	1007ec <vsnprintf+0x194>
  100798:	e594c000 	ldr	ip, [r4]
  10079c:	e1a00006 	mov	r0, r6
  1007a0:	e3a03000 	mov	r3, #0	; 0x0
  1007a4:	ea000002 	b	1007b4 <vsnprintf+0x15c>
  1007a8:	e594c000 	ldr	ip, [r4]
  1007ac:	e1a00006 	mov	r0, r6
  1007b0:	e3a03001 	mov	r3, #1	; 0x1
  1007b4:	e58dc000 	str	ip, [sp]
  1007b8:	e59fc070 	ldr	ip, [pc, #112]	; 100830 <.fixed+0x830>
  1007bc:	e1a0e00f 	mov	lr, pc
  1007c0:	e1a0f00c 	mov	pc, ip
  1007c4:	ea000008 	b	1007ec <vsnprintf+0x194>
  1007c8:	e5941000 	ldr	r1, [r4]
  1007cc:	e59f3060 	ldr	r3, [pc, #96]	; 100834 <.fixed+0x834>
  1007d0:	e1a00006 	mov	r0, r6
  1007d4:	ea000002 	b	1007e4 <vsnprintf+0x18c>
  1007d8:	e5d41000 	ldrb	r1, [r4]
  1007dc:	e59f3054 	ldr	r3, [pc, #84]	; 100838 <.fixed+0x838>
  1007e0:	e1a00006 	mov	r0, r6
  1007e4:	e1a0e00f 	mov	lr, pc
  1007e8:	e1a0f003 	mov	pc, r3
  1007ec:	e2844004 	add	r4, r4, #4	; 0x4
  1007f0:	e0877000 	add	r7, r7, r0
  1007f4:	e2855001 	add	r5, r5, #1	; 0x1
  1007f8:	e0866000 	add	r6, r6, r0
  1007fc:	eaffff9f 	b	100680 <vsnprintf+0x28>
  100800:	e3e00000 	mvn	r0, #0	; 0x0
  100804:	e89da9f8 	ldmia	sp, {r3, r4, r5, r6, r7, r8, fp, sp, pc}
  100808:	e1570008 	cmp	r7, r8
  10080c:	22477001 	subcs	r7, r7, #1	; 0x1
  100810:	33a03000 	movcc	r3, #0	; 0x0
  100814:	23a03000 	movcs	r3, #0	; 0x0
  100818:	e1a00007 	mov	r0, r7
  10081c:	35c63000 	strccb	r3, [r6]
  100820:	25463001 	strcsb	r3, [r6, #-1]
  100824:	e89da9f8 	ldmia	sp, {r3, r4, r5, r6, r7, r8, fp, sp, pc}
  100828:	00100468 	andeqs	r0, r0, r8, ror #8
  10082c:	0010038c 	andeqs	r0, r0, ip, lsl #7
  100830:	00100588 	andeqs	r0, r0, r8, lsl #11
  100834:	00100360 	andeqs	r0, r0, r0, ror #6
  100838:	00100354 	andeqs	r0, r0, r4, asr r3

0010083c <vsprintf>:
  10083c:	e1a03002 	mov	r3, r2
  100840:	e1a02001 	mov	r2, r1
  100844:	e3a01064 	mov	r1, #100	; 0x64
  100848:	eaffff82 	b	100658 <vsnprintf>

0010084c <vfprintf>:
  10084c:	e1a0c00d 	mov	ip, sp
  100850:	e92dd9f0 	stmdb	sp!, {r4, r5, r6, r7, r8, fp, ip, lr, pc}
  100854:	e59f3090 	ldr	r3, [pc, #144]	; 1008ec <.fixed+0x8ec>
  100858:	e24cb004 	sub	fp, ip, #4	; 0x4
  10085c:	e24dd088 	sub	sp, sp, #136	; 0x88
  100860:	e1a0e003 	mov	lr, r3
  100864:	e1a04001 	mov	r4, r1
  100868:	e1a05002 	mov	r5, r2
  10086c:	e1a07000 	mov	r7, r0
  100870:	e8be000f 	ldmia	lr!, {r0, r1, r2, r3}
  100874:	e24bc0a8 	sub	ip, fp, #168	; 0xa8
  100878:	e1a0800c 	mov	r8, ip
  10087c:	e8ac000f 	stmia	ip!, {r0, r1, r2, r3}
  100880:	e8be000f 	ldmia	lr!, {r0, r1, r2, r3}
  100884:	e8ac000f 	stmia	ip!, {r0, r1, r2, r3}
  100888:	e59e3000 	ldr	r3, [lr]
  10088c:	e24b6084 	sub	r6, fp, #132	; 0x84
  100890:	e1a02005 	mov	r2, r5
  100894:	e58c3000 	str	r3, [ip]
  100898:	e1a01004 	mov	r1, r4
  10089c:	e1a00006 	mov	r0, r6
  1008a0:	e59f3048 	ldr	r3, [pc, #72]	; 1008f0 <.fixed+0x8f0>
  1008a4:	e1a0e00f 	mov	lr, pc
  1008a8:	e1a0f003 	mov	pc, r3
  1008ac:	e3500063 	cmp	r0, #99	; 0x63
  1008b0:	e59f203c 	ldr	r2, [pc, #60]	; 1008f4 <.fixed+0x8f4>
  1008b4:	da000006 	ble	1008d4 <vfprintf+0x88>
  1008b8:	e59f3038 	ldr	r3, [pc, #56]	; 1008f8 <.fixed+0x8f8>
  1008bc:	e5933000 	ldr	r3, [r3]
  1008c0:	e1a00008 	mov	r0, r8
  1008c4:	e593100c 	ldr	r1, [r3, #12]
  1008c8:	e1a0e00f 	mov	lr, pc
  1008cc:	e1a0f002 	mov	pc, r2
  1008d0:	eafffffe 	b	1008d0 <vfprintf+0x84>
  1008d4:	e1a00006 	mov	r0, r6
  1008d8:	e1a01007 	mov	r1, r7
  1008dc:	e1a0e00f 	mov	lr, pc
  1008e0:	e1a0f002 	mov	pc, r2
  1008e4:	e24bd020 	sub	sp, fp, #32	; 0x20
  1008e8:	e89da9f0 	ldmia	sp, {r4, r5, r6, r7, r8, fp, sp, pc}
  1008ec:	0010100c 	andeqs	r1, r0, ip
  1008f0:	0010083c 	andeqs	r0, r0, ip, lsr r8
  1008f4:	00100a30 	andeqs	r0, r0, r0, lsr sl
  1008f8:	00200070 	eoreq	r0, r0, r0, ror r0

001008fc <vprintf>:
  1008fc:	e59f3014 	ldr	r3, [pc, #20]	; 100918 <.fixed+0x918>
  100900:	e5933000 	ldr	r3, [r3]
  100904:	e1a0c000 	mov	ip, r0
  100908:	e5930008 	ldr	r0, [r3, #8]
  10090c:	e1a02001 	mov	r2, r1
  100910:	e1a0100c 	mov	r1, ip
  100914:	eaffffcc 	b	10084c <vfprintf>
  100918:	00200070 	eoreq	r0, r0, r0, ror r0

0010091c <printf>:
  10091c:	e1a0c00d 	mov	ip, sp
  100920:	e92d000f 	stmdb	sp!, {r0, r1, r2, r3}
  100924:	e92dd800 	stmdb	sp!, {fp, ip, lr, pc}
  100928:	e24cb014 	sub	fp, ip, #20	; 0x14
  10092c:	e59b0004 	ldr	r0, [fp, #4]
  100930:	e28b1008 	add	r1, fp, #8	; 0x8
  100934:	e59f3008 	ldr	r3, [pc, #8]	; 100944 <.fixed+0x944>
  100938:	e1a0e00f 	mov	lr, pc
  10093c:	e1a0f003 	mov	pc, r3
  100940:	e89da800 	ldmia	sp, {fp, sp, pc}
  100944:	001008fc 	ldreqsh	r0, [r0], -ip

00100948 <DBGU_Configure>:
  100948:	e1a0c00d 	mov	ip, sp
  10094c:	e92dd830 	stmdb	sp!, {r4, r5, fp, ip, lr, pc}
  100950:	e59f4044 	ldr	r4, [pc, #68]	; 10099c <.fixed+0x99c>
  100954:	e3a0300c 	mov	r3, #12	; 0xc
  100958:	e5843000 	str	r3, [r4]
  10095c:	e243300d 	sub	r3, r3, #13	; 0xd
  100960:	e1a05000 	mov	r5, r0
  100964:	e584300c 	str	r3, [r4, #12]
  100968:	e24cb004 	sub	fp, ip, #4	; 0x4
  10096c:	e59f302c 	ldr	r3, [pc, #44]	; 1009a0 <.fixed+0x9a0>
  100970:	e1a01201 	mov	r1, r1, lsl #4
  100974:	e1a00002 	mov	r0, r2
  100978:	e1a0e00f 	mov	lr, pc
  10097c:	e1a0f003 	mov	pc, r3
  100980:	e59f301c 	ldr	r3, [pc, #28]	; 1009a4 <.fixed+0x9a4>
  100984:	e5840020 	str	r0, [r4, #32]
  100988:	e5845004 	str	r5, [r4, #4]
  10098c:	e5843120 	str	r3, [r4, #288]
  100990:	e3a03050 	mov	r3, #80	; 0x50
  100994:	e5843000 	str	r3, [r4]
  100998:	e89da830 	ldmia	sp, {r4, r5, fp, sp, pc}
  10099c:	fffff200 	swinv	0x00fff200
  1009a0:	00100ddc 	ldreqsb	r0, [r0], -ip
  1009a4:	00000202 	andeq	r0, r0, r2, lsl #4

001009a8 <DBGU_PutChar>:
  1009a8:	e20000ff 	and	r0, r0, #255	; 0xff
  1009ac:	e59f2020 	ldr	r2, [pc, #32]	; 1009d4 <.fixed+0x9d4>
  1009b0:	e5923014 	ldr	r3, [r2, #20]
  1009b4:	e3130c02 	tst	r3, #512	; 0x200
  1009b8:	0afffffb 	beq	1009ac <DBGU_PutChar+0x4>
  1009bc:	e582001c 	str	r0, [r2, #28]
  1009c0:	e59f300c 	ldr	r3, [pc, #12]	; 1009d4 <.fixed+0x9d4>
  1009c4:	e5933014 	ldr	r3, [r3, #20]
  1009c8:	e3130c02 	tst	r3, #512	; 0x200
  1009cc:	0afffffb 	beq	1009c0 <DBGU_PutChar+0x18>
  1009d0:	e1a0f00e 	mov	pc, lr
  1009d4:	fffff200 	swinv	0x00fff200

001009d8 <fputc>:
  1009d8:	e1a0c00d 	mov	ip, sp
  1009dc:	e92dd810 	stmdb	sp!, {r4, fp, ip, lr, pc}
  1009e0:	e59f3040 	ldr	r3, [pc, #64]	; 100a28 <.fixed+0xa28>
  1009e4:	e5932000 	ldr	r2, [r3]
  1009e8:	e5923008 	ldr	r3, [r2, #8]
  1009ec:	e1510003 	cmp	r1, r3
  1009f0:	e1a04000 	mov	r4, r0
  1009f4:	e24cb004 	sub	fp, ip, #4	; 0x4
  1009f8:	e20000ff 	and	r0, r0, #255	; 0xff
  1009fc:	0a000003 	beq	100a10 <fputc+0x38>
  100a00:	e592300c 	ldr	r3, [r2, #12]
  100a04:	e1510003 	cmp	r1, r3
  100a08:	e3e03000 	mvn	r3, #0	; 0x0
  100a0c:	1a000003 	bne	100a20 <fputc+0x48>
  100a10:	e59f3014 	ldr	r3, [pc, #20]	; 100a2c <.fixed+0xa2c>
  100a14:	e1a0e00f 	mov	lr, pc
  100a18:	e1a0f003 	mov	pc, r3
  100a1c:	e1a03004 	mov	r3, r4
  100a20:	e1a00003 	mov	r0, r3
  100a24:	e89da810 	ldmia	sp, {r4, fp, sp, pc}
  100a28:	00200070 	eoreq	r0, r0, r0, ror r0
  100a2c:	001009a8 	andeqs	r0, r0, r8, lsr #19

00100a30 <fputs>:
  100a30:	e1a0c00d 	mov	ip, sp
  100a34:	e92dd870 	stmdb	sp!, {r4, r5, r6, fp, ip, lr, pc}
  100a38:	e1a04000 	mov	r4, r0
  100a3c:	e5d00000 	ldrb	r0, [r0]
  100a40:	e24cb004 	sub	fp, ip, #4	; 0x4
  100a44:	e3500000 	cmp	r0, #0	; 0x0
  100a48:	e1a06001 	mov	r6, r1
  100a4c:	e3a05000 	mov	r5, #0	; 0x0
  100a50:	0a000009 	beq	100a7c <fputs+0x4c>
  100a54:	e1a01006 	mov	r1, r6
  100a58:	e59f3024 	ldr	r3, [pc, #36]	; 100a84 <.fixed+0xa84>
  100a5c:	e1a0e00f 	mov	lr, pc
  100a60:	e1a0f003 	mov	pc, r3
  100a64:	e3700001 	cmn	r0, #1	; 0x1
  100a68:	e2855001 	add	r5, r5, #1	; 0x1
  100a6c:	089da870 	ldmeqia	sp, {r4, r5, r6, fp, sp, pc}
  100a70:	e5f40001 	ldrb	r0, [r4, #1]!
  100a74:	e3500000 	cmp	r0, #0	; 0x0
  100a78:	eafffff4 	b	100a50 <fputs+0x20>
  100a7c:	e1a00005 	mov	r0, r5
  100a80:	e89da870 	ldmia	sp, {r4, r5, r6, fp, sp, pc}
  100a84:	001009d8 	ldreqsb	r0, [r0], -r8

00100a88 <putchar>:
  100a88:	e59f3008 	ldr	r3, [pc, #8]	; 100a98 <.fixed+0xa98>
  100a8c:	e5933000 	ldr	r3, [r3]
  100a90:	e5931008 	ldr	r1, [r3, #8]
  100a94:	eaffffcf 	b	1009d8 <fputc>
  100a98:	00200070 	eoreq	r0, r0, r0, ror r0

00100a9c <PIO_Configure>:
  100a9c:	e3510000 	cmp	r1, #0	; 0x0
  100aa0:	e52de004 	str	lr, [sp, #-4]!
  100aa4:	0a000045 	beq	100bc0 <.fixed+0xbc0>
  100aa8:	e5d0c009 	ldrb	ip, [r0, #9]
  100aac:	e35c0004 	cmp	ip, #4	; 0x4
  100ab0:	979ff10c 	ldrls	pc, [pc, ip, lsl #2]
  100ab4:	ea00003c 	b	100bac <.fixed+0xbac>
  100ab8:	00100acc 	andeqs	r0, r0, ip, asr #21
  100abc:	00100aec 	andeqs	r0, r0, ip, ror #21
  100ac0:	00100b10 	andeqs	r0, r0, r0, lsl fp
  100ac4:	00100b5c 	andeqs	r0, r0, ip, asr fp
  100ac8:	00100b5c 	andeqs	r0, r0, ip, asr fp
  100acc:	e5d0300a 	ldrb	r3, [r0, #10]
  100ad0:	e8901004 	ldmia	r0, {r2, ip}
  100ad4:	e3130001 	tst	r3, #1	; 0x1
  100ad8:	e58c2044 	str	r2, [ip, #68]
  100adc:	158c2064 	strne	r2, [ip, #100]
  100ae0:	058c2060 	streq	r2, [ip, #96]
  100ae4:	e58c2070 	str	r2, [ip, #112]
  100ae8:	ea000006 	b	100b08 <.fixed+0xb08>
  100aec:	e5d0300a 	ldrb	r3, [r0, #10]
  100af0:	e8901004 	ldmia	r0, {r2, ip}
  100af4:	e3130001 	tst	r3, #1	; 0x1
  100af8:	e58c2044 	str	r2, [ip, #68]
  100afc:	158c2064 	strne	r2, [ip, #100]
  100b00:	058c2060 	streq	r2, [ip, #96]
  100b04:	e58c2074 	str	r2, [ip, #116]
  100b08:	e58c2004 	str	r2, [ip, #4]
  100b0c:	ea000028 	b	100bb4 <.fixed+0xbb4>
  100b10:	e5d02008 	ldrb	r2, [r0, #8]
  100b14:	e3a03001 	mov	r3, #1	; 0x1
  100b18:	e1a03213 	mov	r3, r3, lsl r2
  100b1c:	e59f20a4 	ldr	r2, [pc, #164]	; 100bc8 <.fixed+0xbc8>
  100b20:	e5823010 	str	r3, [r2, #16]
  100b24:	e5d0300a 	ldrb	r3, [r0, #10]
  100b28:	e8901004 	ldmia	r0, {r2, ip}
  100b2c:	e3130001 	tst	r3, #1	; 0x1
  100b30:	e1a030a3 	mov	r3, r3, lsr #1
  100b34:	e2033001 	and	r3, r3, #1	; 0x1
  100b38:	e58c2044 	str	r2, [ip, #68]
  100b3c:	158c2064 	strne	r2, [ip, #100]
  100b40:	058c2060 	streq	r2, [ip, #96]
  100b44:	e3530000 	cmp	r3, #0	; 0x0
  100b48:	158c2020 	strne	r2, [ip, #32]
  100b4c:	058c2024 	streq	r2, [ip, #36]
  100b50:	e58c2014 	str	r2, [ip, #20]
  100b54:	e58c2000 	str	r2, [ip]
  100b58:	ea000015 	b	100bb4 <.fixed+0xbb4>
  100b5c:	e5d0300a 	ldrb	r3, [r0, #10]
  100b60:	e8904004 	ldmia	r0, {r2, lr}
  100b64:	e35c0004 	cmp	ip, #4	; 0x4
  100b68:	13a0c000 	movne	ip, #0	; 0x0
  100b6c:	03a0c001 	moveq	ip, #1	; 0x1
  100b70:	e3130001 	tst	r3, #1	; 0x1
  100b74:	e1a03123 	mov	r3, r3, lsr #2
  100b78:	e2033001 	and	r3, r3, #1	; 0x1
  100b7c:	e58e2044 	str	r2, [lr, #68]
  100b80:	158e2064 	strne	r2, [lr, #100]
  100b84:	058e2060 	streq	r2, [lr, #96]
  100b88:	e3530000 	cmp	r3, #0	; 0x0
  100b8c:	158e2050 	strne	r2, [lr, #80]
  100b90:	058e2054 	streq	r2, [lr, #84]
  100b94:	e35c0000 	cmp	ip, #0	; 0x0
  100b98:	158e2030 	strne	r2, [lr, #48]
  100b9c:	058e2034 	streq	r2, [lr, #52]
  100ba0:	e58e2010 	str	r2, [lr, #16]
  100ba4:	e58e2000 	str	r2, [lr]
  100ba8:	ea000001 	b	100bb4 <.fixed+0xbb4>
  100bac:	e3a00000 	mov	r0, #0	; 0x0
  100bb0:	e49df004 	ldr	pc, [sp], #4
  100bb4:	e2511001 	subs	r1, r1, #1	; 0x1
  100bb8:	e280000c 	add	r0, r0, #12	; 0xc
  100bbc:	1affffb9 	bne	100aa8 <PIO_Configure+0xc>
  100bc0:	e3a00001 	mov	r0, #1	; 0x1
  100bc4:	e49df004 	ldr	pc, [sp], #4
  100bc8:	fffffc00 	swinv	0x00fffc00

00100bcc <PIO_Set>:
  100bcc:	e890000c 	ldmia	r0, {r2, r3}
  100bd0:	e5832030 	str	r2, [r3, #48]
  100bd4:	e1a0f00e 	mov	pc, lr

00100bd8 <PIO_Clear>:
  100bd8:	e890000c 	ldmia	r0, {r2, r3}
  100bdc:	e5832034 	str	r2, [r3, #52]
  100be0:	e1a0f00e 	mov	pc, lr

00100be4 <PIO_Get>:
  100be4:	e5d03009 	ldrb	r3, [r0, #9]
  100be8:	e2433003 	sub	r3, r3, #3	; 0x3
  100bec:	e3530001 	cmp	r3, #1	; 0x1
  100bf0:	e5903000 	ldr	r3, [r0]
  100bf4:	e5900004 	ldr	r0, [r0, #4]
  100bf8:	95900038 	ldrls	r0, [r0, #56]
  100bfc:	8590003c 	ldrhi	r0, [r0, #60]
  100c00:	e0100003 	ands	r0, r0, r3
  100c04:	13a00001 	movne	r0, #1	; 0x1
  100c08:	e1a0f00e 	mov	pc, lr

00100c0c <BOARD_GetRemap>:
  100c0c:	e52de004 	str	lr, [sp, #-4]!
  100c10:	e3a0c602 	mov	ip, #2097152	; 0x200000
  100c14:	e59c1000 	ldr	r1, [ip]
  100c18:	e3a0e000 	mov	lr, #0	; 0x0
  100c1c:	e59e3000 	ldr	r3, [lr]
  100c20:	e2812001 	add	r2, r1, #1	; 0x1
  100c24:	e1530002 	cmp	r3, r2
  100c28:	e3a00001 	mov	r0, #1	; 0x1
  100c2c:	11a0000e 	movne	r0, lr
  100c30:	e58c2000 	str	r2, [ip]
  100c34:	058c1000 	streq	r1, [ip]
  100c38:	158c1000 	strne	r1, [ip]
  100c3c:	e49df004 	ldr	pc, [sp], #4

00100c40 <BOARD_RemapRam>:
  100c40:	e1a0c00d 	mov	ip, sp
  100c44:	e59f3024 	ldr	r3, [pc, #36]	; 100c70 <.fixed+0xc70>
  100c48:	e92dd800 	stmdb	sp!, {fp, ip, lr, pc}
  100c4c:	e24cb004 	sub	fp, ip, #4	; 0x4
  100c50:	e1a0e00f 	mov	lr, pc
  100c54:	e1a0f003 	mov	pc, r3
  100c58:	e20000ff 	and	r0, r0, #255	; 0xff
  100c5c:	e3500001 	cmp	r0, #1	; 0x1
  100c60:	13a02001 	movne	r2, #1	; 0x1
  100c64:	13e030ff 	mvnne	r3, #255	; 0xff
  100c68:	15832000 	strne	r2, [r3]
  100c6c:	e89da800 	ldmia	sp, {fp, sp, pc}
  100c70:	00100c0c 	andeqs	r0, r0, ip, lsl #24

00100c74 <defaultSpuriousHandler>:
  100c74:	eafffffe 	b	100c74 <defaultSpuriousHandler>

00100c78 <defaultFiqHandler>:
  100c78:	eafffffe 	b	100c78 <defaultFiqHandler>

00100c7c <defaultIrqHandler>:
  100c7c:	eafffffe 	b	100c7c <defaultIrqHandler>

00100c80 <LowLevelInit>:
  100c80:	e3a02c01 	mov	r2, #256	; 0x100
  100c84:	e1a0c00d 	mov	ip, sp
  100c88:	e3e0309f 	mvn	r3, #159	; 0x9f
  100c8c:	e92dd800 	stmdb	sp!, {fp, ip, lr, pc}
  100c90:	e4832010 	str	r2, [r3], #16
  100c94:	e5832000 	str	r2, [r3]
  100c98:	e59f2114 	ldr	r2, [pc, #276]	; 100db4 <.fixed+0xdb4>
  100c9c:	e2433e37 	sub	r3, r3, #880	; 0x370
  100ca0:	e5832020 	str	r2, [r3, #32]
  100ca4:	e24cb004 	sub	fp, ip, #4	; 0x4
  100ca8:	e59f2108 	ldr	r2, [pc, #264]	; 100db8 <.fixed+0xdb8>
  100cac:	e5923068 	ldr	r3, [r2, #104]
  100cb0:	e3130001 	tst	r3, #1	; 0x1
  100cb4:	0afffffb 	beq	100ca8 <LowLevelInit+0x28>
  100cb8:	e59f30fc 	ldr	r3, [pc, #252]	; 100dbc <.fixed+0xdbc>
  100cbc:	e582302c 	str	r3, [r2, #44]
  100cc0:	e59f30f0 	ldr	r3, [pc, #240]	; 100db8 <.fixed+0xdb8>
  100cc4:	e5933068 	ldr	r3, [r3, #104]
  100cc8:	e3130004 	tst	r3, #4	; 0x4
  100ccc:	0afffffb 	beq	100cc0 <LowLevelInit+0x40>
  100cd0:	e59f20e0 	ldr	r2, [pc, #224]	; 100db8 <.fixed+0xdb8>
  100cd4:	e5923068 	ldr	r3, [r2, #104]
  100cd8:	e3130008 	tst	r3, #8	; 0x8
  100cdc:	0afffffb 	beq	100cd0 <LowLevelInit+0x50>
  100ce0:	e3a03004 	mov	r3, #4	; 0x4
  100ce4:	e5823030 	str	r3, [r2, #48]
  100ce8:	e59f20c8 	ldr	r2, [pc, #200]	; 100db8 <.fixed+0xdb8>
  100cec:	e5923068 	ldr	r3, [r2, #104]
  100cf0:	e3130008 	tst	r3, #8	; 0x8
  100cf4:	0afffffb 	beq	100ce8 <LowLevelInit+0x68>
  100cf8:	e5923030 	ldr	r3, [r2, #48]
  100cfc:	e3833003 	orr	r3, r3, #3	; 0x3
  100d00:	e5823030 	str	r3, [r2, #48]
  100d04:	e59f30ac 	ldr	r3, [pc, #172]	; 100db8 <.fixed+0xdb8>
  100d08:	e5933068 	ldr	r3, [r3, #104]
  100d0c:	e3130008 	tst	r3, #8	; 0x8
  100d10:	0afffffb 	beq	100d04 <LowLevelInit+0x84>
  100d14:	e59f30a4 	ldr	r3, [pc, #164]	; 100dc0 <.fixed+0xdc0>
  100d18:	e3e02000 	mvn	r2, #0	; 0x0
  100d1c:	e5832124 	str	r2, [r3, #292]
  100d20:	e59f209c 	ldr	r2, [pc, #156]	; 100dc4 <.fixed+0xdc4>
  100d24:	e5832080 	str	r2, [r3, #128]
  100d28:	e3a01001 	mov	r1, #1	; 0x1
  100d2c:	e2813001 	add	r3, r1, #1	; 0x1
  100d30:	e1a02101 	mov	r2, r1, lsl #2
  100d34:	e20310ff 	and	r1, r3, #255	; 0xff
  100d38:	e59f3088 	ldr	r3, [pc, #136]	; 100dc8 <.fixed+0xdc8>
  100d3c:	e351001e 	cmp	r1, #30	; 0x1e
  100d40:	e5023f80 	str	r3, [r2, #-3968]
  100d44:	9afffff8 	bls	100d2c <LowLevelInit+0xac>
  100d48:	e59f207c 	ldr	r2, [pc, #124]	; 100dcc <.fixed+0xdcc>
  100d4c:	e59f306c 	ldr	r3, [pc, #108]	; 100dc0 <.fixed+0xdc0>
  100d50:	e5832134 	str	r2, [r3, #308]
  100d54:	e3a01000 	mov	r1, #0	; 0x0
  100d58:	e2813001 	add	r3, r1, #1	; 0x1
  100d5c:	e20310ff 	and	r1, r3, #255	; 0xff
  100d60:	e59f2058 	ldr	r2, [pc, #88]	; 100dc0 <.fixed+0xdc0>
  100d64:	e3a03000 	mov	r3, #0	; 0x0
  100d68:	e3510007 	cmp	r1, #7	; 0x7
  100d6c:	e5823130 	str	r3, [r2, #304]
  100d70:	9afffff8 	bls	100d58 <LowLevelInit+0xd8>
  100d74:	e2833001 	add	r3, r3, #1	; 0x1
  100d78:	e5823138 	str	r3, [r2, #312]
  100d7c:	e59f304c 	ldr	r3, [pc, #76]	; 100dd0 <.fixed+0xdd0>
  100d80:	e2822a09 	add	r2, r2, #36864	; 0x9000
  100d84:	e5832004 	str	r2, [r3, #4]
  100d88:	e59f3044 	ldr	r3, [pc, #68]	; 100dd4 <.fixed+0xdd4>
  100d8c:	e1a0e00f 	mov	lr, pc
  100d90:	e1a0f003 	mov	pc, r3
  100d94:	e59f203c 	ldr	r2, [pc, #60]	; 100dd8 <.fixed+0xdd8>
  100d98:	e5923000 	ldr	r3, [r2]
  100d9c:	e3c33803 	bic	r3, r3, #196608	; 0x30000
  100da0:	e4823010 	str	r3, [r2], #16
  100da4:	e5923000 	ldr	r3, [r2]
  100da8:	e3c33402 	bic	r3, r3, #33554432	; 0x2000000
  100dac:	e5823000 	str	r3, [r2]
  100db0:	e89da800 	ldmia	sp, {fp, sp, pc}
  100db4:	00004001 	andeq	r4, r0, r1
  100db8:	fffffc00 	swinv	0x00fffc00
  100dbc:	1048100e 	subne	r1, r8, lr
  100dc0:	fffff000 	swinv	0x00fff000
  100dc4:	00100c78 	andeqs	r0, r0, r8, ror ip
  100dc8:	00100c7c 	andeqs	r0, r0, ip, ror ip
  100dcc:	00100c74 	andeqs	r0, r0, r4, ror ip
  100dd0:	fffffd40 	swinv	0x00fffd40
  100dd4:	00100c40 	andeqs	r0, r0, r0, asr #24
  100dd8:	fffffd20 	swinv	0x00fffd20

00100ddc <__udivsi3>:
  100ddc:	e2512001 	subs	r2, r1, #1	; 0x1
  100de0:	012fff1e 	bxeq	lr
  100de4:	3a000036 	bcc	100ec4 <__udivsi3+0xe8>
  100de8:	e1500001 	cmp	r0, r1
  100dec:	9a000022 	bls	100e7c <__udivsi3+0xa0>
  100df0:	e1110002 	tst	r1, r2
  100df4:	0a000023 	beq	100e88 <__udivsi3+0xac>
  100df8:	e311020e 	tst	r1, #-536870912	; 0xe0000000
  100dfc:	01a01181 	moveq	r1, r1, lsl #3
  100e00:	03a03008 	moveq	r3, #8	; 0x8
  100e04:	13a03001 	movne	r3, #1	; 0x1
  100e08:	e3510201 	cmp	r1, #268435456	; 0x10000000
  100e0c:	31510000 	cmpcc	r1, r0
  100e10:	31a01201 	movcc	r1, r1, lsl #4
  100e14:	31a03203 	movcc	r3, r3, lsl #4
  100e18:	3afffffa 	bcc	100e08 <__udivsi3+0x2c>
  100e1c:	e3510102 	cmp	r1, #-2147483648	; 0x80000000
  100e20:	31510000 	cmpcc	r1, r0
  100e24:	31a01081 	movcc	r1, r1, lsl #1
  100e28:	31a03083 	movcc	r3, r3, lsl #1
  100e2c:	3afffffa 	bcc	100e1c <__udivsi3+0x40>
  100e30:	e3a02000 	mov	r2, #0	; 0x0
  100e34:	e1500001 	cmp	r0, r1
  100e38:	20400001 	subcs	r0, r0, r1
  100e3c:	21822003 	orrcs	r2, r2, r3
  100e40:	e15000a1 	cmp	r0, r1, lsr #1
  100e44:	204000a1 	subcs	r0, r0, r1, lsr #1
  100e48:	218220a3 	orrcs	r2, r2, r3, lsr #1
  100e4c:	e1500121 	cmp	r0, r1, lsr #2
  100e50:	20400121 	subcs	r0, r0, r1, lsr #2
  100e54:	21822123 	orrcs	r2, r2, r3, lsr #2
  100e58:	e15001a1 	cmp	r0, r1, lsr #3
  100e5c:	204001a1 	subcs	r0, r0, r1, lsr #3
  100e60:	218221a3 	orrcs	r2, r2, r3, lsr #3
  100e64:	e3500000 	cmp	r0, #0	; 0x0
  100e68:	11b03223 	movnes	r3, r3, lsr #4
  100e6c:	11a01221 	movne	r1, r1, lsr #4
  100e70:	1affffef 	bne	100e34 <__udivsi3+0x58>
  100e74:	e1a00002 	mov	r0, r2
  100e78:	e12fff1e 	bx	lr
  100e7c:	03a00001 	moveq	r0, #1	; 0x1
  100e80:	13a00000 	movne	r0, #0	; 0x0
  100e84:	e12fff1e 	bx	lr
  100e88:	e3510801 	cmp	r1, #65536	; 0x10000
  100e8c:	21a01821 	movcs	r1, r1, lsr #16
  100e90:	23a02010 	movcs	r2, #16	; 0x10
  100e94:	33a02000 	movcc	r2, #0	; 0x0
  100e98:	e3510c01 	cmp	r1, #256	; 0x100
  100e9c:	21a01421 	movcs	r1, r1, lsr #8
  100ea0:	22822008 	addcs	r2, r2, #8	; 0x8
  100ea4:	e3510010 	cmp	r1, #16	; 0x10
  100ea8:	21a01221 	movcs	r1, r1, lsr #4
  100eac:	22822004 	addcs	r2, r2, #4	; 0x4
  100eb0:	e3510004 	cmp	r1, #4	; 0x4
  100eb4:	82822003 	addhi	r2, r2, #3	; 0x3
  100eb8:	908220a1 	addls	r2, r2, r1, lsr #1
  100ebc:	e1a00230 	mov	r0, r0, lsr r2
  100ec0:	e12fff1e 	bx	lr
  100ec4:	e52de004 	str	lr, [sp, #-4]!
  100ec8:	eb000034 	bl	100fa0 <__div0>
  100ecc:	e3a00000 	mov	r0, #0	; 0x0
  100ed0:	e49df004 	ldr	pc, [sp], #4

00100ed4 <__umodsi3>:
  100ed4:	e2512001 	subs	r2, r1, #1	; 0x1
  100ed8:	3a00002c 	bcc	100f90 <__umodsi3+0xbc>
  100edc:	11500001 	cmpne	r0, r1
  100ee0:	03a00000 	moveq	r0, #0	; 0x0
  100ee4:	81110002 	tsthi	r1, r2
  100ee8:	00000002 	andeq	r0, r0, r2
  100eec:	912fff1e 	bxls	lr
  100ef0:	e3a02000 	mov	r2, #0	; 0x0
  100ef4:	e3510201 	cmp	r1, #268435456	; 0x10000000
  100ef8:	31510000 	cmpcc	r1, r0
  100efc:	31a01201 	movcc	r1, r1, lsl #4
  100f00:	32822004 	addcc	r2, r2, #4	; 0x4
  100f04:	3afffffa 	bcc	100ef4 <__umodsi3+0x20>
  100f08:	e3510102 	cmp	r1, #-2147483648	; 0x80000000
  100f0c:	31510000 	cmpcc	r1, r0
  100f10:	31a01081 	movcc	r1, r1, lsl #1
  100f14:	32822001 	addcc	r2, r2, #1	; 0x1
  100f18:	3afffffa 	bcc	100f08 <__umodsi3+0x34>
  100f1c:	e2522003 	subs	r2, r2, #3	; 0x3
  100f20:	ba00000e 	blt	100f60 <__umodsi3+0x8c>
  100f24:	e1500001 	cmp	r0, r1
  100f28:	20400001 	subcs	r0, r0, r1
  100f2c:	e15000a1 	cmp	r0, r1, lsr #1
  100f30:	204000a1 	subcs	r0, r0, r1, lsr #1
  100f34:	e1500121 	cmp	r0, r1, lsr #2
  100f38:	20400121 	subcs	r0, r0, r1, lsr #2
  100f3c:	e15001a1 	cmp	r0, r1, lsr #3
  100f40:	204001a1 	subcs	r0, r0, r1, lsr #3
  100f44:	e3500001 	cmp	r0, #1	; 0x1
  100f48:	e1a01221 	mov	r1, r1, lsr #4
  100f4c:	a2522004 	subges	r2, r2, #4	; 0x4
  100f50:	aafffff3 	bge	100f24 <__umodsi3+0x50>
  100f54:	e3120003 	tst	r2, #3	; 0x3
  100f58:	13300000 	teqne	r0, #0	; 0x0
  100f5c:	0a00000a 	beq	100f8c <__umodsi3+0xb8>
  100f60:	e3720002 	cmn	r2, #2	; 0x2
  100f64:	ba000006 	blt	100f84 <__umodsi3+0xb0>
  100f68:	0a000002 	beq	100f78 <__umodsi3+0xa4>
  100f6c:	e1500001 	cmp	r0, r1
  100f70:	20400001 	subcs	r0, r0, r1
  100f74:	e1a010a1 	mov	r1, r1, lsr #1
  100f78:	e1500001 	cmp	r0, r1
  100f7c:	20400001 	subcs	r0, r0, r1
  100f80:	e1a010a1 	mov	r1, r1, lsr #1
  100f84:	e1500001 	cmp	r0, r1
  100f88:	20400001 	subcs	r0, r0, r1
  100f8c:	e12fff1e 	bx	lr
  100f90:	e52de004 	str	lr, [sp, #-4]!
  100f94:	eb000001 	bl	100fa0 <__div0>
  100f98:	e3a00000 	mov	r0, #0	; 0x0
  100f9c:	e49df004 	ldr	pc, [sp], #4

00100fa0 <__div0>:
  100fa0:	e12fff1e 	bx	lr

00100fa4 <PIN_led_utca1>:
  100fa4:	04000000 fffff600 00000303              ............

00100fb0 <PIN_led_utca0>:
  100fb0:	40000000 fffff400 00000302              ...@........

00100fbc <PIN_main_nrst>:
  100fbc:	00000002 fffff400 00000302              ............

00100fc8 <PIN_utca_pwron>:
  100fc8:	00200000 fffff400 00000302              .. .........

00100fd4 <PIN_flash_serial_sel>:
  100fd4:	00000040 fffff400 00000402              @...........

00100fe0 <PIN_dtxd_bootsel>:
  100fe0:	10000000 fffff400 00010202 18000000     ................
  100ff0:	fffff400 00000002 63726f66 61735f65     ........force_sa
  101000:	3a61626d 0a642520 00000000 69647473     mba: %d.....stdi
  101010:	3a632e6f 636e6920 73616572 414d2065     o.c: increase MA
  101020:	54535f58 474e4952 5a49535f 000d0a45     X_STRING_SIZE...
