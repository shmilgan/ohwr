
bin/mch_wd_dummy-sram.elf:     file format elf32-littlearm

Disassembly of section .fixed:

00200000 <_sfixed>:
  200000:	e59ff060 	ldr	pc, [pc, #96]	; 200068 <.fixed+0x68>

00200004 <undefVector>:
  200004:	eafffffe 	b	200004 <undefVector>

00200008 <swiVector>:
  200008:	eafffffe 	b	200008 <swiVector>

0020000c <prefetchAbortVector>:
  20000c:	eafffffe 	b	20000c <prefetchAbortVector>

00200010 <dataAbortVector>:
  200010:	eafffffe 	b	200010 <dataAbortVector>

00200014 <reservedVector>:
  200014:	eafffffe 	b	200014 <reservedVector>

00200018 <irqVector>:
  200018:	ea000000 	b	200020 <irqHandler>

0020001c <fiqHandler>:
  20001c:	eafffffe 	b	20001c <fiqHandler>

00200020 <irqHandler>:
  200020:	e24ee004 	sub	lr, lr, #4	; 0x4
  200024:	e92d4000 	stmdb	sp!, {lr}
  200028:	e14fe000 	mrs	lr, SPSR
  20002c:	e92d4001 	stmdb	sp!, {r0, lr}
  200030:	e59fe034 	ldr	lr, [pc, #52]	; 20006c <.fixed+0x6c>
  200034:	e59e0100 	ldr	r0, [lr, #256]
  200038:	e58ee100 	str	lr, [lr, #256]
  20003c:	e321f013 	msr	CPSR_c, #19	; 0x13
  200040:	e92d501e 	stmdb	sp!, {r1, r2, r3, r4, ip, lr}
  200044:	e1a0e00f 	mov	lr, pc
  200048:	e12fff10 	bx	r0
  20004c:	e8bd501e 	ldmia	sp!, {r1, r2, r3, r4, ip, lr}
  200050:	e321f092 	msr	CPSR_c, #146	; 0x92
  200054:	e59fe010 	ldr	lr, [pc, #16]	; 20006c <.fixed+0x6c>
  200058:	e58ee130 	str	lr, [lr, #304]
  20005c:	e8bd4001 	ldmia	sp!, {r0, lr}
  200060:	e16ff00e 	msr	SPSR_fsxc, lr
  200064:	e8fd8000 	ldmia	sp!, {pc}^
  200068:	00200070 	eoreq	r0, r0, r0, ror r0
  20006c:	fffff000 	swinv	0x00fff000

00200070 <entry>:
  200070:	e59f006c 	ldr	r0, [pc, #108]	; 2000e4 <.fixed+0xe4>
  200074:	e59ff06c 	ldr	pc, [pc, #108]	; 2000e8 <.fixed+0xe8>
  200078:	e59f406c 	ldr	r4, [pc, #108]	; 2000ec <.fixed+0xec>
  20007c:	e1a0d004 	mov	sp, r4
  200080:	e59f0068 	ldr	r0, [pc, #104]	; 2000f0 <.fixed+0xf0>
  200084:	e1a0e00f 	mov	lr, pc
  200088:	e12fff10 	bx	r0
  20008c:	e59f0060 	ldr	r0, [pc, #96]	; 2000f4 <.fixed+0xf4>
  200090:	e59f1060 	ldr	r1, [pc, #96]	; 2000f8 <.fixed+0xf8>
  200094:	e59f2060 	ldr	r2, [pc, #96]	; 2000fc <.fixed+0xfc>
  200098:	e1510002 	cmp	r1, r2
  20009c:	34903004 	ldrcc	r3, [r0], #4
  2000a0:	34813004 	strcc	r3, [r1], #4
  2000a4:	3afffffb 	bcc	200098 <entry+0x28>
  2000a8:	e59f0050 	ldr	r0, [pc, #80]	; 200100 <.fixed+0x100>
  2000ac:	e59f1050 	ldr	r1, [pc, #80]	; 200104 <.fixed+0x104>
  2000b0:	e3a02000 	mov	r2, #0	; 0x0
  2000b4:	e1500001 	cmp	r0, r1
  2000b8:	34802004 	strcc	r2, [r0], #4
  2000bc:	3afffffc 	bcc	2000b4 <entry+0x44>
  2000c0:	e321f0d2 	msr	CPSR_c, #210	; 0xd2
  2000c4:	e1a0d004 	mov	sp, r4
  2000c8:	e2444060 	sub	r4, r4, #96	; 0x60
  2000cc:	e321f053 	msr	CPSR_c, #83	; 0x53
  2000d0:	e1a0d004 	mov	sp, r4
  2000d4:	e59f002c 	ldr	r0, [pc, #44]	; 200108 <.fixed+0x108>
  2000d8:	e1a0e00f 	mov	lr, pc
  2000dc:	e12fff10 	bx	r0
  2000e0:	eafffffe 	b	2000e0 <entry+0x70>
  2000e4:	00200000 	eoreq	r0, r0, r0
  2000e8:	00200078 	eoreq	r0, r0, r8, ror r0
  2000ec:	00220000 	eoreq	r0, r2, r0
  2000f0:	00200cf0 	streqd	r0, [r0], -r0
  2000f4:	00201420 	eoreq	r1, r0, r0, lsr #8
	...
  200100:	00201420 	eoreq	r1, r0, r0, lsr #8
  200104:	00201420 	eoreq	r1, r0, r0, lsr #8
  200108:	002001e8 	eoreq	r0, r0, r8, ror #3
  20010c:	e1a00000 	nop			(mov r0,r0)

00200110 <delay>:
  200110:	e24dd004 	sub	sp, sp, #4	; 0x4
  200114:	e2500001 	subs	r0, r0, #1	; 0x1
  200118:	3a00000b 	bcc	20014c <delay+0x3c>
  20011c:	e3a03000 	mov	r3, #0	; 0x0
  200120:	e58d3000 	str	r3, [sp]
  200124:	e59d3000 	ldr	r3, [sp]
  200128:	e59f2024 	ldr	r2, [pc, #36]	; 200154 <.fixed+0x154>
  20012c:	e1530002 	cmp	r3, r2
  200130:	cafffff7 	bgt	200114 <delay+0x4>
  200134:	e1a00000 	nop			(mov r0,r0)
  200138:	e59d3000 	ldr	r3, [sp]
  20013c:	e2833001 	add	r3, r3, #1	; 0x1
  200140:	e58d3000 	str	r3, [sp]
  200144:	e59d3000 	ldr	r3, [sp]
  200148:	eafffff7 	b	20012c <delay+0x1c>
  20014c:	e28dd004 	add	sp, sp, #4	; 0x4
  200150:	e1a0f00e 	mov	pc, lr
  200154:	0000270f 	andeq	r2, r0, pc, lsl #14

00200158 <io_init>:
  200158:	e1a0c00d 	mov	ip, sp
  20015c:	e92dd810 	stmdb	sp!, {r4, fp, ip, lr, pc}
  200160:	e3a01001 	mov	r1, #1	; 0x1
  200164:	e24cb004 	sub	fp, ip, #4	; 0x4
  200168:	e59f405c 	ldr	r4, [pc, #92]	; 2001cc <.fixed+0x1cc>
  20016c:	e59f005c 	ldr	r0, [pc, #92]	; 2001d0 <.fixed+0x1d0>
  200170:	e1a0e00f 	mov	lr, pc
  200174:	e1a0f004 	mov	pc, r4
  200178:	e3a01001 	mov	r1, #1	; 0x1
  20017c:	e59f0050 	ldr	r0, [pc, #80]	; 2001d4 <.fixed+0x1d4>
  200180:	e1a0e00f 	mov	lr, pc
  200184:	e1a0f004 	mov	pc, r4
  200188:	e3a01001 	mov	r1, #1	; 0x1
  20018c:	e59f0044 	ldr	r0, [pc, #68]	; 2001d8 <.fixed+0x1d8>
  200190:	e1a0e00f 	mov	lr, pc
  200194:	e1a0f004 	mov	pc, r4
  200198:	e3a01001 	mov	r1, #1	; 0x1
  20019c:	e59f0038 	ldr	r0, [pc, #56]	; 2001dc <.fixed+0x1dc>
  2001a0:	e1a0e00f 	mov	lr, pc
  2001a4:	e1a0f004 	mov	pc, r4
  2001a8:	e3a01001 	mov	r1, #1	; 0x1
  2001ac:	e59f002c 	ldr	r0, [pc, #44]	; 2001e0 <.fixed+0x1e0>
  2001b0:	e1a0e00f 	mov	lr, pc
  2001b4:	e1a0f004 	mov	pc, r4
  2001b8:	e3a01001 	mov	r1, #1	; 0x1
  2001bc:	e59f0020 	ldr	r0, [pc, #32]	; 2001e4 <.fixed+0x1e4>
  2001c0:	e1a0e00f 	mov	lr, pc
  2001c4:	e1a0f004 	mov	pc, r4
  2001c8:	e89da810 	ldmia	sp, {r4, fp, sp, pc}
  2001cc:	00200b0c 	eoreq	r0, r0, ip, lsl #22
  2001d0:	00201020 	eoreq	r1, r0, r0, lsr #32
  2001d4:	00201014 	eoreq	r1, r0, r4, lsl r0
  2001d8:	00201038 	eoreq	r1, r0, r8, lsr r0
  2001dc:	00201044 	eoreq	r1, r0, r4, asr #32
  2001e0:	0020102c 	eoreq	r1, r0, ip, lsr #32
  2001e4:	00201050 	eoreq	r1, r0, r0, asr r0

002001e8 <main>:
  2001e8:	e1a0c00d 	mov	ip, sp
  2001ec:	e92dd870 	stmdb	sp!, {r4, r5, r6, fp, ip, lr, pc}
  2001f0:	e24cb004 	sub	fp, ip, #4	; 0x4
  2001f4:	e24dd00c 	sub	sp, sp, #12	; 0xc
  2001f8:	e59f3178 	ldr	r3, [pc, #376]	; 200378 <.fixed+0x378>
  2001fc:	e1a0e00f 	mov	lr, pc
  200200:	e1a0f003 	mov	pc, r3
  200204:	e59f3170 	ldr	r3, [pc, #368]	; 20037c <.fixed+0x37c>
  200208:	e59f0170 	ldr	r0, [pc, #368]	; 200380 <.fixed+0x380>
  20020c:	e1a0e00f 	mov	lr, pc
  200210:	e1a0f003 	mov	pc, r3
  200214:	e59f3168 	ldr	r3, [pc, #360]	; 200384 <.fixed+0x384>
  200218:	e31000ff 	tst	r0, #255	; 0xff
  20021c:	e8930007 	ldmia	r3, {r0, r1, r2}
  200220:	e24b3024 	sub	r3, fp, #36	; 0x24
  200224:	e8830007 	stmia	r3, {r0, r1, r2}
  200228:	e1a00003 	mov	r0, r3
  20022c:	e3a01001 	mov	r1, #1	; 0x1
  200230:	e59f3150 	ldr	r3, [pc, #336]	; 200388 <.fixed+0x388>
  200234:	13a04000 	movne	r4, #0	; 0x0
  200238:	03a04001 	moveq	r4, #1	; 0x1
  20023c:	e1a0e00f 	mov	lr, pc
  200240:	e1a0f003 	mov	pc, r3
  200244:	e3a00b02 	mov	r0, #2048	; 0x800
  200248:	e59f113c 	ldr	r1, [pc, #316]	; 20038c <.fixed+0x38c>
  20024c:	e59f213c 	ldr	r2, [pc, #316]	; 200390 <.fixed+0x390>
  200250:	e59f313c 	ldr	r3, [pc, #316]	; 200394 <.fixed+0x394>
  200254:	e1a0e00f 	mov	lr, pc
  200258:	e1a0f003 	mov	pc, r3
  20025c:	e1a01004 	mov	r1, r4
  200260:	e59f0130 	ldr	r0, [pc, #304]	; 200398 <.fixed+0x398>
  200264:	e59f3130 	ldr	r3, [pc, #304]	; 20039c <.fixed+0x39c>
  200268:	e1a0e00f 	mov	lr, pc
  20026c:	e1a0f003 	mov	pc, r3
  200270:	e3540000 	cmp	r4, #0	; 0x0
  200274:	e59f5124 	ldr	r5, [pc, #292]	; 2003a0 <.fixed+0x3a0>
  200278:	e59f4124 	ldr	r4, [pc, #292]	; 2003a4 <.fixed+0x3a4>
  20027c:	e59f6124 	ldr	r6, [pc, #292]	; 2003a8 <.fixed+0x3a8>
  200280:	0a000012 	beq	2002d0 <main+0xe8>
  200284:	e59f0120 	ldr	r0, [pc, #288]	; 2003ac <.fixed+0x3ac>
  200288:	e1a0e00f 	mov	lr, pc
  20028c:	e1a0f004 	mov	pc, r4
  200290:	e59f0118 	ldr	r0, [pc, #280]	; 2003b0 <.fixed+0x3b0>
  200294:	e1a0e00f 	mov	lr, pc
  200298:	e1a0f004 	mov	pc, r4
  20029c:	e3a00014 	mov	r0, #20	; 0x14
  2002a0:	e1a0e00f 	mov	lr, pc
  2002a4:	e1a0f005 	mov	pc, r5
  2002a8:	e59f0104 	ldr	r0, [pc, #260]	; 2003b4 <.fixed+0x3b4>
  2002ac:	e1a0e00f 	mov	lr, pc
  2002b0:	e1a0f004 	mov	pc, r4
  2002b4:	e3a000c8 	mov	r0, #200	; 0xc8
  2002b8:	e1a0e00f 	mov	lr, pc
  2002bc:	e1a0f005 	mov	pc, r5
  2002c0:	e59f00e4 	ldr	r0, [pc, #228]	; 2003ac <.fixed+0x3ac>
  2002c4:	e1a0e00f 	mov	lr, pc
  2002c8:	e1a0f006 	mov	pc, r6
  2002cc:	ea00000b 	b	200300 <main+0x118>
  2002d0:	e59f00d4 	ldr	r0, [pc, #212]	; 2003ac <.fixed+0x3ac>
  2002d4:	e1a0e00f 	mov	lr, pc
  2002d8:	e1a0f006 	mov	pc, r6
  2002dc:	e59f00cc 	ldr	r0, [pc, #204]	; 2003b0 <.fixed+0x3b0>
  2002e0:	e1a0e00f 	mov	lr, pc
  2002e4:	e1a0f004 	mov	pc, r4
  2002e8:	e3a00014 	mov	r0, #20	; 0x14
  2002ec:	e1a0e00f 	mov	lr, pc
  2002f0:	e1a0f005 	mov	pc, r5
  2002f4:	e59f00b8 	ldr	r0, [pc, #184]	; 2003b4 <.fixed+0x3b4>
  2002f8:	e1a0e00f 	mov	lr, pc
  2002fc:	e1a0f004 	mov	pc, r4
  200300:	e59f30b0 	ldr	r3, [pc, #176]	; 2003b8 <.fixed+0x3b8>
  200304:	e3a0002c 	mov	r0, #44	; 0x2c
  200308:	e1a0e00f 	mov	lr, pc
  20030c:	e1a0f003 	mov	pc, r3
  200310:	e59f608c 	ldr	r6, [pc, #140]	; 2003a4 <.fixed+0x3a4>
  200314:	e59f00a0 	ldr	r0, [pc, #160]	; 2003bc <.fixed+0x3bc>
  200318:	e59f4080 	ldr	r4, [pc, #128]	; 2003a0 <.fixed+0x3a0>
  20031c:	e1a0e00f 	mov	lr, pc
  200320:	e1a0f006 	mov	pc, r6
  200324:	e3a0000a 	mov	r0, #10	; 0xa
  200328:	e1a0e00f 	mov	lr, pc
  20032c:	e1a0f004 	mov	pc, r4
  200330:	e59f5070 	ldr	r5, [pc, #112]	; 2003a8 <.fixed+0x3a8>
  200334:	e59f0080 	ldr	r0, [pc, #128]	; 2003bc <.fixed+0x3bc>
  200338:	e1a0e00f 	mov	lr, pc
  20033c:	e1a0f005 	mov	pc, r5
  200340:	e3a0000a 	mov	r0, #10	; 0xa
  200344:	e1a0e00f 	mov	lr, pc
  200348:	e1a0f004 	mov	pc, r4
  20034c:	e59f006c 	ldr	r0, [pc, #108]	; 2003c0 <.fixed+0x3c0>
  200350:	e1a0e00f 	mov	lr, pc
  200354:	e1a0f006 	mov	pc, r6
  200358:	e3a0000a 	mov	r0, #10	; 0xa
  20035c:	e1a0e00f 	mov	lr, pc
  200360:	e1a0f004 	mov	pc, r4
  200364:	e59f0054 	ldr	r0, [pc, #84]	; 2003c0 <.fixed+0x3c0>
  200368:	e1a0e00f 	mov	lr, pc
  20036c:	e1a0f005 	mov	pc, r5
  200370:	e3a0000a 	mov	r0, #10	; 0xa
  200374:	eaffffdf 	b	2002f8 <main+0x110>
  200378:	00200158 	eoreq	r0, r0, r8, asr r1
  20037c:	00200c54 	eoreq	r0, r0, r4, asr ip
  200380:	00201050 	eoreq	r1, r0, r0, asr r0
  200384:	0020105c 	eoreq	r1, r0, ip, asr r0
  200388:	00200b0c 	eoreq	r0, r0, ip, lsl #22
  20038c:	0001c200 	andeq	ip, r1, r0, lsl #4
  200390:	02dc6c00 	sbceqs	r6, ip, #0	; 0x0
  200394:	002009b8 	streqh	r0, [r0], -r8
  200398:	00201068 	eoreq	r1, r0, r8, rrx
  20039c:	0020098c 	eoreq	r0, r0, ip, lsl #19
  2003a0:	00200110 	eoreq	r0, r0, r0, lsl r1
  2003a4:	00200c3c 	eoreq	r0, r0, ip, lsr ip
  2003a8:	00200c48 	eoreq	r0, r0, r8, asr #24
  2003ac:	00201044 	eoreq	r1, r0, r4, asr #32
  2003b0:	00201038 	eoreq	r1, r0, r8, lsr r0
  2003b4:	0020102c 	eoreq	r1, r0, ip, lsr #32
  2003b8:	00200af8 	streqd	r0, [r0], -r8
  2003bc:	00201020 	eoreq	r1, r0, r0, lsr #32
  2003c0:	00201014 	eoreq	r1, r0, r4, lsl r0

002003c4 <PutChar>:
  2003c4:	e5c01000 	strb	r1, [r0]
  2003c8:	e3a00001 	mov	r0, #1	; 0x1
  2003cc:	e1a0f00e 	mov	pc, lr

002003d0 <PutString>:
  2003d0:	e5d13000 	ldrb	r3, [r1]
  2003d4:	e3530000 	cmp	r3, #0	; 0x0
  2003d8:	e1a02000 	mov	r2, r0
  2003dc:	e3a00000 	mov	r0, #0	; 0x0
  2003e0:	01a0f00e 	moveq	pc, lr
  2003e4:	e4c23001 	strb	r3, [r2], #1
  2003e8:	e5f13001 	ldrb	r3, [r1, #1]!
  2003ec:	e3530000 	cmp	r3, #0	; 0x0
  2003f0:	e2800001 	add	r0, r0, #1	; 0x1
  2003f4:	1afffffa 	bne	2003e4 <PutString+0x14>
  2003f8:	e1a0f00e 	mov	pc, lr

002003fc <PutUnsignedInt>:
  2003fc:	e1a0c00d 	mov	ip, sp
  200400:	e92dd9f0 	stmdb	sp!, {r4, r5, r6, r7, r8, fp, ip, lr, pc}
  200404:	e1a08003 	mov	r8, r3
  200408:	e1a03001 	mov	r3, r1
  20040c:	e24cb004 	sub	fp, ip, #4	; 0x4
  200410:	e1a05000 	mov	r5, r0
  200414:	e3a0100a 	mov	r1, #10	; 0xa
  200418:	e1a00008 	mov	r0, r8
  20041c:	e20370ff 	and	r7, r3, #255	; 0xff
  200420:	e59f30a0 	ldr	r3, [pc, #160]	; 2004c8 <.fixed+0x4c8>
  200424:	e2424001 	sub	r4, r2, #1	; 0x1
  200428:	e1a0e00f 	mov	lr, pc
  20042c:	e1a0f003 	mov	pc, r3
  200430:	e3500000 	cmp	r0, #0	; 0x0
  200434:	e3a06000 	mov	r6, #0	; 0x0
  200438:	0a000009 	beq	200464 <PutUnsignedInt+0x68>
  20043c:	e1a03000 	mov	r3, r0
  200440:	e1a01007 	mov	r1, r7
  200444:	e1a00005 	mov	r0, r5
  200448:	e1a02004 	mov	r2, r4
  20044c:	e59fc078 	ldr	ip, [pc, #120]	; 2004cc <.fixed+0x4cc>
  200450:	e1a0e00f 	mov	lr, pc
  200454:	e1a0f00c 	mov	pc, ip
  200458:	e1a06000 	mov	r6, r0
  20045c:	e0855000 	add	r5, r5, r0
  200460:	ea00000b 	b	200494 <PutUnsignedInt+0x98>
  200464:	e3540000 	cmp	r4, #0	; 0x0
  200468:	da000009 	ble	200494 <PutUnsignedInt+0x98>
  20046c:	e1a00005 	mov	r0, r5
  200470:	e1a01007 	mov	r1, r7
  200474:	e59f3054 	ldr	r3, [pc, #84]	; 2004d0 <.fixed+0x4d0>
  200478:	e1a0e00f 	mov	lr, pc
  20047c:	e1a0f003 	mov	pc, r3
  200480:	e2444001 	sub	r4, r4, #1	; 0x1
  200484:	e3540000 	cmp	r4, #0	; 0x0
  200488:	e2855001 	add	r5, r5, #1	; 0x1
  20048c:	e2866001 	add	r6, r6, #1	; 0x1
  200490:	eafffff4 	b	200468 <PutUnsignedInt+0x6c>
  200494:	e1a00008 	mov	r0, r8
  200498:	e3a0100a 	mov	r1, #10	; 0xa
  20049c:	e59f3030 	ldr	r3, [pc, #48]	; 2004d4 <.fixed+0x4d4>
  2004a0:	e1a0e00f 	mov	lr, pc
  2004a4:	e1a0f003 	mov	pc, r3
  2004a8:	e2801030 	add	r1, r0, #48	; 0x30
  2004ac:	e20110ff 	and	r1, r1, #255	; 0xff
  2004b0:	e1a00005 	mov	r0, r5
  2004b4:	e59f3014 	ldr	r3, [pc, #20]	; 2004d0 <.fixed+0x4d0>
  2004b8:	e1a0e00f 	mov	lr, pc
  2004bc:	e1a0f003 	mov	pc, r3
  2004c0:	e0860000 	add	r0, r6, r0
  2004c4:	e89da9f0 	ldmia	sp, {r4, r5, r6, r7, r8, fp, sp, pc}
  2004c8:	00200e4c 	eoreq	r0, r0, ip, asr #28
  2004cc:	002003fc 	streqd	r0, [r0], -ip
  2004d0:	002003c4 	eoreq	r0, r0, r4, asr #7
  2004d4:	00200f44 	eoreq	r0, r0, r4, asr #30

002004d8 <PutSignedInt>:
  2004d8:	e1a0c00d 	mov	ip, sp
  2004dc:	e92dddf0 	stmdb	sp!, {r4, r5, r6, r7, r8, sl, fp, ip, lr, pc}
  2004e0:	e023afc3 	eor	sl, r3, r3, asr #31
  2004e4:	e04aafc3 	sub	sl, sl, r3, asr #31
  2004e8:	e1a06003 	mov	r6, r3
  2004ec:	e1a03001 	mov	r3, r1
  2004f0:	e24cb004 	sub	fp, ip, #4	; 0x4
  2004f4:	e1a05000 	mov	r5, r0
  2004f8:	e3a0100a 	mov	r1, #10	; 0xa
  2004fc:	e1a0000a 	mov	r0, sl
  200500:	e20380ff 	and	r8, r3, #255	; 0xff
  200504:	e59f30dc 	ldr	r3, [pc, #220]	; 2005e8 <.fixed+0x5e8>
  200508:	e2424001 	sub	r4, r2, #1	; 0x1
  20050c:	e1a0e00f 	mov	lr, pc
  200510:	e1a0f003 	mov	pc, r3
  200514:	e3500000 	cmp	r0, #0	; 0x0
  200518:	e3a07000 	mov	r7, #0	; 0x0
  20051c:	0a00000d 	beq	200558 <PutSignedInt+0x80>
  200520:	e1560007 	cmp	r6, r7
  200524:	b2603000 	rsblt	r3, r0, #0	; 0x0
  200528:	a1a03000 	movge	r3, r0
  20052c:	e59fc0b8 	ldr	ip, [pc, #184]	; 2005ec <.fixed+0x5ec>
  200530:	e1a00005 	mov	r0, r5
  200534:	b1a01008 	movlt	r1, r8
  200538:	b1a02004 	movlt	r2, r4
  20053c:	a1a01008 	movge	r1, r8
  200540:	a1a02004 	movge	r2, r4
  200544:	e1a0e00f 	mov	lr, pc
  200548:	e1a0f00c 	mov	pc, ip
  20054c:	e1a07000 	mov	r7, r0
  200550:	e0855000 	add	r5, r5, r0
  200554:	ea000016 	b	2005b4 <PutSignedInt+0xdc>
  200558:	e3560000 	cmp	r6, #0	; 0x0
  20055c:	b2444001 	sublt	r4, r4, #1	; 0x1
  200560:	e3540000 	cmp	r4, #0	; 0x0
  200564:	da000009 	ble	200590 <PutSignedInt+0xb8>
  200568:	e1a00005 	mov	r0, r5
  20056c:	e1a01008 	mov	r1, r8
  200570:	e59f3078 	ldr	r3, [pc, #120]	; 2005f0 <.fixed+0x5f0>
  200574:	e1a0e00f 	mov	lr, pc
  200578:	e1a0f003 	mov	pc, r3
  20057c:	e2444001 	sub	r4, r4, #1	; 0x1
  200580:	e3540000 	cmp	r4, #0	; 0x0
  200584:	e2855001 	add	r5, r5, #1	; 0x1
  200588:	e2877001 	add	r7, r7, #1	; 0x1
  20058c:	eafffff4 	b	200564 <PutSignedInt+0x8c>
  200590:	e3560000 	cmp	r6, #0	; 0x0
  200594:	aa000006 	bge	2005b4 <PutSignedInt+0xdc>
  200598:	e1a00005 	mov	r0, r5
  20059c:	e3a0102d 	mov	r1, #45	; 0x2d
  2005a0:	e59f3048 	ldr	r3, [pc, #72]	; 2005f0 <.fixed+0x5f0>
  2005a4:	e1a0e00f 	mov	lr, pc
  2005a8:	e1a0f003 	mov	pc, r3
  2005ac:	e2855001 	add	r5, r5, #1	; 0x1
  2005b0:	e0877000 	add	r7, r7, r0
  2005b4:	e1a0000a 	mov	r0, sl
  2005b8:	e3a0100a 	mov	r1, #10	; 0xa
  2005bc:	e59f3030 	ldr	r3, [pc, #48]	; 2005f4 <.fixed+0x5f4>
  2005c0:	e1a0e00f 	mov	lr, pc
  2005c4:	e1a0f003 	mov	pc, r3
  2005c8:	e2801030 	add	r1, r0, #48	; 0x30
  2005cc:	e20110ff 	and	r1, r1, #255	; 0xff
  2005d0:	e1a00005 	mov	r0, r5
  2005d4:	e59f3014 	ldr	r3, [pc, #20]	; 2005f0 <.fixed+0x5f0>
  2005d8:	e1a0e00f 	mov	lr, pc
  2005dc:	e1a0f003 	mov	pc, r3
  2005e0:	e0870000 	add	r0, r7, r0
  2005e4:	e89dadf0 	ldmia	sp, {r4, r5, r6, r7, r8, sl, fp, sp, pc}
  2005e8:	00200e4c 	eoreq	r0, r0, ip, asr #28
  2005ec:	002004d8 	ldreqd	r0, [r0], -r8
  2005f0:	002003c4 	eoreq	r0, r0, r4, asr #7
  2005f4:	00200f44 	eoreq	r0, r0, r4, asr #30

002005f8 <PutHexa>:
  2005f8:	e1a0c00d 	mov	ip, sp
  2005fc:	e92dddf0 	stmdb	sp!, {r4, r5, r6, r7, r8, sl, fp, ip, lr, pc}
  200600:	e24cb004 	sub	fp, ip, #4	; 0x4
  200604:	e24dd004 	sub	sp, sp, #4	; 0x4
  200608:	e59b8004 	ldr	r8, [fp, #4]
  20060c:	e1b0c228 	movs	ip, r8, lsr #4
  200610:	e20160ff 	and	r6, r1, #255	; 0xff
  200614:	e203a0ff 	and	sl, r3, #255	; 0xff
  200618:	e2425001 	sub	r5, r2, #1	; 0x1
  20061c:	e1a04000 	mov	r4, r0
  200620:	e3a07000 	mov	r7, #0	; 0x0
  200624:	0a000009 	beq	200650 <PutHexa+0x58>
  200628:	e58dc000 	str	ip, [sp]
  20062c:	e1a01006 	mov	r1, r6
  200630:	e1a02005 	mov	r2, r5
  200634:	e1a0300a 	mov	r3, sl
  200638:	e59fc080 	ldr	ip, [pc, #128]	; 2006c0 <.fixed+0x6c0>
  20063c:	e1a0e00f 	mov	lr, pc
  200640:	e1a0f00c 	mov	pc, ip
  200644:	e1a07000 	mov	r7, r0
  200648:	e0844000 	add	r4, r4, r0
  20064c:	ea00000b 	b	200680 <PutHexa+0x88>
  200650:	e3550000 	cmp	r5, #0	; 0x0
  200654:	da000009 	ble	200680 <PutHexa+0x88>
  200658:	e1a00004 	mov	r0, r4
  20065c:	e1a01006 	mov	r1, r6
  200660:	e59f305c 	ldr	r3, [pc, #92]	; 2006c4 <.fixed+0x6c4>
  200664:	e1a0e00f 	mov	lr, pc
  200668:	e1a0f003 	mov	pc, r3
  20066c:	e2455001 	sub	r5, r5, #1	; 0x1
  200670:	e3550000 	cmp	r5, #0	; 0x0
  200674:	e2844001 	add	r4, r4, #1	; 0x1
  200678:	e2877001 	add	r7, r7, #1	; 0x1
  20067c:	eafffff4 	b	200654 <PutHexa+0x5c>
  200680:	e208100f 	and	r1, r8, #15	; 0xf
  200684:	e3510009 	cmp	r1, #9	; 0x9
  200688:	91a00004 	movls	r0, r4
  20068c:	92811030 	addls	r1, r1, #48	; 0x30
  200690:	959f302c 	ldrls	r3, [pc, #44]	; 2006c4 <.fixed+0x6c4>
  200694:	9a000005 	bls	2006b0 <PutHexa+0xb8>
  200698:	e35a0000 	cmp	sl, #0	; 0x0
  20069c:	e59f3020 	ldr	r3, [pc, #32]	; 2006c4 <.fixed+0x6c4>
  2006a0:	11a00004 	movne	r0, r4
  2006a4:	12811037 	addne	r1, r1, #55	; 0x37
  2006a8:	01a00004 	moveq	r0, r4
  2006ac:	02811057 	addeq	r1, r1, #87	; 0x57
  2006b0:	e1a0e00f 	mov	lr, pc
  2006b4:	e1a0f003 	mov	pc, r3
  2006b8:	e2870001 	add	r0, r7, #1	; 0x1
  2006bc:	e89dadf8 	ldmia	sp, {r3, r4, r5, r6, r7, r8, sl, fp, sp, pc}
  2006c0:	002005f8 	streqd	r0, [r0], -r8
  2006c4:	002003c4 	eoreq	r0, r0, r4, asr #7

002006c8 <vsnprintf>:
  2006c8:	e1a0c00d 	mov	ip, sp
  2006cc:	e92dd9f0 	stmdb	sp!, {r4, r5, r6, r7, r8, fp, ip, lr, pc}
  2006d0:	e24cb004 	sub	fp, ip, #4	; 0x4
  2006d4:	e24dd004 	sub	sp, sp, #4	; 0x4
  2006d8:	e2506000 	subs	r6, r0, #0	; 0x0
  2006dc:	e3a07000 	mov	r7, #0	; 0x0
  2006e0:	15c67000 	strneb	r7, [r6]
  2006e4:	e1a08001 	mov	r8, r1
  2006e8:	e1a05002 	mov	r5, r2
  2006ec:	e1a04003 	mov	r4, r3
  2006f0:	e5d53000 	ldrb	r3, [r5]
  2006f4:	e20320ff 	and	r2, r3, #255	; 0xff
  2006f8:	e3520000 	cmp	r2, #0	; 0x0
  2006fc:	11570008 	cmpne	r7, r8
  200700:	2a00005c 	bcs	200878 <vsnprintf+0x1b0>
  200704:	e3520025 	cmp	r2, #37	; 0x25
  200708:	14c63001 	strneb	r3, [r6], #1
  20070c:	12855001 	addne	r5, r5, #1	; 0x1
  200710:	1a000004 	bne	200728 <vsnprintf+0x60>
  200714:	e5d53001 	ldrb	r3, [r5, #1]
  200718:	e3530025 	cmp	r3, #37	; 0x25
  20071c:	1a000003 	bne	200730 <vsnprintf+0x68>
  200720:	e4c62001 	strb	r2, [r6], #1
  200724:	e2855002 	add	r5, r5, #2	; 0x2
  200728:	e2877001 	add	r7, r7, #1	; 0x1
  20072c:	eaffffef 	b	2006f0 <vsnprintf+0x28>
  200730:	e5f53001 	ldrb	r3, [r5, #1]!
  200734:	e3530030 	cmp	r3, #48	; 0x30
  200738:	02855001 	addeq	r5, r5, #1	; 0x1
  20073c:	e5d50000 	ldrb	r0, [r5]
  200740:	e3a01020 	mov	r1, #32	; 0x20
  200744:	01a01003 	moveq	r1, r3
  200748:	e2403030 	sub	r3, r0, #48	; 0x30
  20074c:	e3a02000 	mov	r2, #0	; 0x0
  200750:	e3530009 	cmp	r3, #9	; 0x9
  200754:	8a000007 	bhi	200778 <vsnprintf+0xb0>
  200758:	e0823102 	add	r3, r2, r2, lsl #2
  20075c:	e0803083 	add	r3, r0, r3, lsl #1
  200760:	e5f50001 	ldrb	r0, [r5, #1]!
  200764:	e2433030 	sub	r3, r3, #48	; 0x30
  200768:	e2402030 	sub	r2, r0, #48	; 0x30
  20076c:	e3520009 	cmp	r2, #9	; 0x9
  200770:	e20320ff 	and	r2, r3, #255	; 0xff
  200774:	eafffff6 	b	200754 <vsnprintf+0x8c>
  200778:	e0873002 	add	r3, r7, r2
  20077c:	e1530008 	cmp	r3, r8
  200780:	80673008 	rsbhi	r3, r7, r8
  200784:	820320ff 	andhi	r2, r3, #255	; 0xff
  200788:	e5d53000 	ldrb	r3, [r5]
  20078c:	e3530069 	cmp	r3, #105	; 0x69
  200790:	0a000012 	beq	2007e0 <vsnprintf+0x118>
  200794:	ca000008 	bgt	2007bc <vsnprintf+0xf4>
  200798:	e3530063 	cmp	r3, #99	; 0x63
  20079c:	0a000029 	beq	200848 <vsnprintf+0x180>
  2007a0:	ca000002 	bgt	2007b0 <vsnprintf+0xe8>
  2007a4:	e3530058 	cmp	r3, #88	; 0x58
  2007a8:	0a00001a 	beq	200818 <vsnprintf+0x150>
  2007ac:	ea00002f 	b	200870 <vsnprintf+0x1a8>
  2007b0:	e3530064 	cmp	r3, #100	; 0x64
  2007b4:	0a000009 	beq	2007e0 <vsnprintf+0x118>
  2007b8:	ea00002c 	b	200870 <vsnprintf+0x1a8>
  2007bc:	e3530075 	cmp	r3, #117	; 0x75
  2007c0:	0a00000a 	beq	2007f0 <vsnprintf+0x128>
  2007c4:	ca000002 	bgt	2007d4 <vsnprintf+0x10c>
  2007c8:	e3530073 	cmp	r3, #115	; 0x73
  2007cc:	0a000019 	beq	200838 <vsnprintf+0x170>
  2007d0:	ea000026 	b	200870 <vsnprintf+0x1a8>
  2007d4:	e3530078 	cmp	r3, #120	; 0x78
  2007d8:	0a00000a 	beq	200808 <vsnprintf+0x140>
  2007dc:	ea000023 	b	200870 <vsnprintf+0x1a8>
  2007e0:	e5943000 	ldr	r3, [r4]
  2007e4:	e59fc0ac 	ldr	ip, [pc, #172]	; 200898 <.fixed+0x898>
  2007e8:	e1a00006 	mov	r0, r6
  2007ec:	ea000002 	b	2007fc <vsnprintf+0x134>
  2007f0:	e5943000 	ldr	r3, [r4]
  2007f4:	e59fc0a0 	ldr	ip, [pc, #160]	; 20089c <.fixed+0x89c>
  2007f8:	e1a00006 	mov	r0, r6
  2007fc:	e1a0e00f 	mov	lr, pc
  200800:	e1a0f00c 	mov	pc, ip
  200804:	ea000014 	b	20085c <vsnprintf+0x194>
  200808:	e594c000 	ldr	ip, [r4]
  20080c:	e1a00006 	mov	r0, r6
  200810:	e3a03000 	mov	r3, #0	; 0x0
  200814:	ea000002 	b	200824 <vsnprintf+0x15c>
  200818:	e594c000 	ldr	ip, [r4]
  20081c:	e1a00006 	mov	r0, r6
  200820:	e3a03001 	mov	r3, #1	; 0x1
  200824:	e58dc000 	str	ip, [sp]
  200828:	e59fc070 	ldr	ip, [pc, #112]	; 2008a0 <.fixed+0x8a0>
  20082c:	e1a0e00f 	mov	lr, pc
  200830:	e1a0f00c 	mov	pc, ip
  200834:	ea000008 	b	20085c <vsnprintf+0x194>
  200838:	e5941000 	ldr	r1, [r4]
  20083c:	e59f3060 	ldr	r3, [pc, #96]	; 2008a4 <.fixed+0x8a4>
  200840:	e1a00006 	mov	r0, r6
  200844:	ea000002 	b	200854 <vsnprintf+0x18c>
  200848:	e5d41000 	ldrb	r1, [r4]
  20084c:	e59f3054 	ldr	r3, [pc, #84]	; 2008a8 <.fixed+0x8a8>
  200850:	e1a00006 	mov	r0, r6
  200854:	e1a0e00f 	mov	lr, pc
  200858:	e1a0f003 	mov	pc, r3
  20085c:	e2844004 	add	r4, r4, #4	; 0x4
  200860:	e0877000 	add	r7, r7, r0
  200864:	e2855001 	add	r5, r5, #1	; 0x1
  200868:	e0866000 	add	r6, r6, r0
  20086c:	eaffff9f 	b	2006f0 <vsnprintf+0x28>
  200870:	e3e00000 	mvn	r0, #0	; 0x0
  200874:	e89da9f8 	ldmia	sp, {r3, r4, r5, r6, r7, r8, fp, sp, pc}
  200878:	e1570008 	cmp	r7, r8
  20087c:	22477001 	subcs	r7, r7, #1	; 0x1
  200880:	33a03000 	movcc	r3, #0	; 0x0
  200884:	23a03000 	movcs	r3, #0	; 0x0
  200888:	e1a00007 	mov	r0, r7
  20088c:	35c63000 	strccb	r3, [r6]
  200890:	25463001 	strcsb	r3, [r6, #-1]
  200894:	e89da9f8 	ldmia	sp, {r3, r4, r5, r6, r7, r8, fp, sp, pc}
  200898:	002004d8 	ldreqd	r0, [r0], -r8
  20089c:	002003fc 	streqd	r0, [r0], -ip
  2008a0:	002005f8 	streqd	r0, [r0], -r8
  2008a4:	002003d0 	ldreqd	r0, [r0], -r0
  2008a8:	002003c4 	eoreq	r0, r0, r4, asr #7

002008ac <vsprintf>:
  2008ac:	e1a03002 	mov	r3, r2
  2008b0:	e1a02001 	mov	r2, r1
  2008b4:	e3a01064 	mov	r1, #100	; 0x64
  2008b8:	eaffff82 	b	2006c8 <vsnprintf>

002008bc <vfprintf>:
  2008bc:	e1a0c00d 	mov	ip, sp
  2008c0:	e92dd9f0 	stmdb	sp!, {r4, r5, r6, r7, r8, fp, ip, lr, pc}
  2008c4:	e59f3090 	ldr	r3, [pc, #144]	; 20095c <.fixed+0x95c>
  2008c8:	e24cb004 	sub	fp, ip, #4	; 0x4
  2008cc:	e24dd088 	sub	sp, sp, #136	; 0x88
  2008d0:	e1a0e003 	mov	lr, r3
  2008d4:	e1a04001 	mov	r4, r1
  2008d8:	e1a05002 	mov	r5, r2
  2008dc:	e1a07000 	mov	r7, r0
  2008e0:	e8be000f 	ldmia	lr!, {r0, r1, r2, r3}
  2008e4:	e24bc0a8 	sub	ip, fp, #168	; 0xa8
  2008e8:	e1a0800c 	mov	r8, ip
  2008ec:	e8ac000f 	stmia	ip!, {r0, r1, r2, r3}
  2008f0:	e8be000f 	ldmia	lr!, {r0, r1, r2, r3}
  2008f4:	e8ac000f 	stmia	ip!, {r0, r1, r2, r3}
  2008f8:	e59e3000 	ldr	r3, [lr]
  2008fc:	e24b6084 	sub	r6, fp, #132	; 0x84
  200900:	e1a02005 	mov	r2, r5
  200904:	e58c3000 	str	r3, [ip]
  200908:	e1a01004 	mov	r1, r4
  20090c:	e1a00006 	mov	r0, r6
  200910:	e59f3048 	ldr	r3, [pc, #72]	; 200960 <.fixed+0x960>
  200914:	e1a0e00f 	mov	lr, pc
  200918:	e1a0f003 	mov	pc, r3
  20091c:	e3500063 	cmp	r0, #99	; 0x63
  200920:	e59f203c 	ldr	r2, [pc, #60]	; 200964 <.fixed+0x964>
  200924:	da000006 	ble	200944 <vfprintf+0x88>
  200928:	e59f3038 	ldr	r3, [pc, #56]	; 200968 <.fixed+0x968>
  20092c:	e5933000 	ldr	r3, [r3]
  200930:	e1a00008 	mov	r0, r8
  200934:	e593100c 	ldr	r1, [r3, #12]
  200938:	e1a0e00f 	mov	lr, pc
  20093c:	e1a0f002 	mov	pc, r2
  200940:	eafffffe 	b	200940 <vfprintf+0x84>
  200944:	e1a00006 	mov	r0, r6
  200948:	e1a01007 	mov	r1, r7
  20094c:	e1a0e00f 	mov	lr, pc
  200950:	e1a0f002 	mov	pc, r2
  200954:	e24bd020 	sub	sp, fp, #32	; 0x20
  200958:	e89da9f0 	ldmia	sp, {r4, r5, r6, r7, r8, fp, sp, pc}
  20095c:	0020107c 	eoreq	r1, r0, ip, ror r0
  200960:	002008ac 	eoreq	r0, r0, ip, lsr #17
  200964:	00200aa0 	eoreq	r0, r0, r0, lsr #21
  200968:	002010a0 	eoreq	r1, r0, r0, lsr #1

0020096c <vprintf>:
  20096c:	e59f3014 	ldr	r3, [pc, #20]	; 200988 <.fixed+0x988>
  200970:	e5933000 	ldr	r3, [r3]
  200974:	e1a0c000 	mov	ip, r0
  200978:	e5930008 	ldr	r0, [r3, #8]
  20097c:	e1a02001 	mov	r2, r1
  200980:	e1a0100c 	mov	r1, ip
  200984:	eaffffcc 	b	2008bc <vfprintf>
  200988:	002010a0 	eoreq	r1, r0, r0, lsr #1

0020098c <printf>:
  20098c:	e1a0c00d 	mov	ip, sp
  200990:	e92d000f 	stmdb	sp!, {r0, r1, r2, r3}
  200994:	e92dd800 	stmdb	sp!, {fp, ip, lr, pc}
  200998:	e24cb014 	sub	fp, ip, #20	; 0x14
  20099c:	e59b0004 	ldr	r0, [fp, #4]
  2009a0:	e28b1008 	add	r1, fp, #8	; 0x8
  2009a4:	e59f3008 	ldr	r3, [pc, #8]	; 2009b4 <.fixed+0x9b4>
  2009a8:	e1a0e00f 	mov	lr, pc
  2009ac:	e1a0f003 	mov	pc, r3
  2009b0:	e89da800 	ldmia	sp, {fp, sp, pc}
  2009b4:	0020096c 	eoreq	r0, r0, ip, ror #18

002009b8 <DBGU_Configure>:
  2009b8:	e1a0c00d 	mov	ip, sp
  2009bc:	e92dd830 	stmdb	sp!, {r4, r5, fp, ip, lr, pc}
  2009c0:	e59f4044 	ldr	r4, [pc, #68]	; 200a0c <.fixed+0xa0c>
  2009c4:	e3a0300c 	mov	r3, #12	; 0xc
  2009c8:	e5843000 	str	r3, [r4]
  2009cc:	e243300d 	sub	r3, r3, #13	; 0xd
  2009d0:	e1a05000 	mov	r5, r0
  2009d4:	e584300c 	str	r3, [r4, #12]
  2009d8:	e24cb004 	sub	fp, ip, #4	; 0x4
  2009dc:	e59f302c 	ldr	r3, [pc, #44]	; 200a10 <.fixed+0xa10>
  2009e0:	e1a01201 	mov	r1, r1, lsl #4
  2009e4:	e1a00002 	mov	r0, r2
  2009e8:	e1a0e00f 	mov	lr, pc
  2009ec:	e1a0f003 	mov	pc, r3
  2009f0:	e59f301c 	ldr	r3, [pc, #28]	; 200a14 <.fixed+0xa14>
  2009f4:	e5840020 	str	r0, [r4, #32]
  2009f8:	e5845004 	str	r5, [r4, #4]
  2009fc:	e5843120 	str	r3, [r4, #288]
  200a00:	e3a03050 	mov	r3, #80	; 0x50
  200a04:	e5843000 	str	r3, [r4]
  200a08:	e89da830 	ldmia	sp, {r4, r5, fp, sp, pc}
  200a0c:	fffff200 	swinv	0x00fff200
  200a10:	00200e4c 	eoreq	r0, r0, ip, asr #28
  200a14:	00000202 	andeq	r0, r0, r2, lsl #4

00200a18 <DBGU_PutChar>:
  200a18:	e20000ff 	and	r0, r0, #255	; 0xff
  200a1c:	e59f2020 	ldr	r2, [pc, #32]	; 200a44 <.fixed+0xa44>
  200a20:	e5923014 	ldr	r3, [r2, #20]
  200a24:	e3130c02 	tst	r3, #512	; 0x200
  200a28:	0afffffb 	beq	200a1c <DBGU_PutChar+0x4>
  200a2c:	e582001c 	str	r0, [r2, #28]
  200a30:	e59f300c 	ldr	r3, [pc, #12]	; 200a44 <.fixed+0xa44>
  200a34:	e5933014 	ldr	r3, [r3, #20]
  200a38:	e3130c02 	tst	r3, #512	; 0x200
  200a3c:	0afffffb 	beq	200a30 <DBGU_PutChar+0x18>
  200a40:	e1a0f00e 	mov	pc, lr
  200a44:	fffff200 	swinv	0x00fff200

00200a48 <fputc>:
  200a48:	e1a0c00d 	mov	ip, sp
  200a4c:	e92dd810 	stmdb	sp!, {r4, fp, ip, lr, pc}
  200a50:	e59f3040 	ldr	r3, [pc, #64]	; 200a98 <.fixed+0xa98>
  200a54:	e5932000 	ldr	r2, [r3]
  200a58:	e5923008 	ldr	r3, [r2, #8]
  200a5c:	e1510003 	cmp	r1, r3
  200a60:	e1a04000 	mov	r4, r0
  200a64:	e24cb004 	sub	fp, ip, #4	; 0x4
  200a68:	e20000ff 	and	r0, r0, #255	; 0xff
  200a6c:	0a000003 	beq	200a80 <fputc+0x38>
  200a70:	e592300c 	ldr	r3, [r2, #12]
  200a74:	e1510003 	cmp	r1, r3
  200a78:	e3e03000 	mvn	r3, #0	; 0x0
  200a7c:	1a000003 	bne	200a90 <fputc+0x48>
  200a80:	e59f3014 	ldr	r3, [pc, #20]	; 200a9c <.fixed+0xa9c>
  200a84:	e1a0e00f 	mov	lr, pc
  200a88:	e1a0f003 	mov	pc, r3
  200a8c:	e1a03004 	mov	r3, r4
  200a90:	e1a00003 	mov	r0, r3
  200a94:	e89da810 	ldmia	sp, {r4, fp, sp, pc}
  200a98:	002010a0 	eoreq	r1, r0, r0, lsr #1
  200a9c:	00200a18 	eoreq	r0, r0, r8, lsl sl

00200aa0 <fputs>:
  200aa0:	e1a0c00d 	mov	ip, sp
  200aa4:	e92dd870 	stmdb	sp!, {r4, r5, r6, fp, ip, lr, pc}
  200aa8:	e1a04000 	mov	r4, r0
  200aac:	e5d00000 	ldrb	r0, [r0]
  200ab0:	e24cb004 	sub	fp, ip, #4	; 0x4
  200ab4:	e3500000 	cmp	r0, #0	; 0x0
  200ab8:	e1a06001 	mov	r6, r1
  200abc:	e3a05000 	mov	r5, #0	; 0x0
  200ac0:	0a000009 	beq	200aec <fputs+0x4c>
  200ac4:	e1a01006 	mov	r1, r6
  200ac8:	e59f3024 	ldr	r3, [pc, #36]	; 200af4 <.fixed+0xaf4>
  200acc:	e1a0e00f 	mov	lr, pc
  200ad0:	e1a0f003 	mov	pc, r3
  200ad4:	e3700001 	cmn	r0, #1	; 0x1
  200ad8:	e2855001 	add	r5, r5, #1	; 0x1
  200adc:	089da870 	ldmeqia	sp, {r4, r5, r6, fp, sp, pc}
  200ae0:	e5f40001 	ldrb	r0, [r4, #1]!
  200ae4:	e3500000 	cmp	r0, #0	; 0x0
  200ae8:	eafffff4 	b	200ac0 <fputs+0x20>
  200aec:	e1a00005 	mov	r0, r5
  200af0:	e89da870 	ldmia	sp, {r4, r5, r6, fp, sp, pc}
  200af4:	00200a48 	eoreq	r0, r0, r8, asr #20

00200af8 <putchar>:
  200af8:	e59f3008 	ldr	r3, [pc, #8]	; 200b08 <.fixed+0xb08>
  200afc:	e5933000 	ldr	r3, [r3]
  200b00:	e5931008 	ldr	r1, [r3, #8]
  200b04:	eaffffcf 	b	200a48 <fputc>
  200b08:	002010a0 	eoreq	r1, r0, r0, lsr #1

00200b0c <PIO_Configure>:
  200b0c:	e3510000 	cmp	r1, #0	; 0x0
  200b10:	e52de004 	str	lr, [sp, #-4]!
  200b14:	0a000045 	beq	200c30 <.fixed+0xc30>
  200b18:	e5d0c009 	ldrb	ip, [r0, #9]
  200b1c:	e35c0004 	cmp	ip, #4	; 0x4
  200b20:	979ff10c 	ldrls	pc, [pc, ip, lsl #2]
  200b24:	ea00003c 	b	200c1c <.fixed+0xc1c>
  200b28:	00200b3c 	eoreq	r0, r0, ip, lsr fp
  200b2c:	00200b5c 	eoreq	r0, r0, ip, asr fp
  200b30:	00200b80 	eoreq	r0, r0, r0, lsl #23
  200b34:	00200bcc 	eoreq	r0, r0, ip, asr #23
  200b38:	00200bcc 	eoreq	r0, r0, ip, asr #23
  200b3c:	e5d0300a 	ldrb	r3, [r0, #10]
  200b40:	e8901004 	ldmia	r0, {r2, ip}
  200b44:	e3130001 	tst	r3, #1	; 0x1
  200b48:	e58c2044 	str	r2, [ip, #68]
  200b4c:	158c2064 	strne	r2, [ip, #100]
  200b50:	058c2060 	streq	r2, [ip, #96]
  200b54:	e58c2070 	str	r2, [ip, #112]
  200b58:	ea000006 	b	200b78 <.fixed+0xb78>
  200b5c:	e5d0300a 	ldrb	r3, [r0, #10]
  200b60:	e8901004 	ldmia	r0, {r2, ip}
  200b64:	e3130001 	tst	r3, #1	; 0x1
  200b68:	e58c2044 	str	r2, [ip, #68]
  200b6c:	158c2064 	strne	r2, [ip, #100]
  200b70:	058c2060 	streq	r2, [ip, #96]
  200b74:	e58c2074 	str	r2, [ip, #116]
  200b78:	e58c2004 	str	r2, [ip, #4]
  200b7c:	ea000028 	b	200c24 <.fixed+0xc24>
  200b80:	e5d02008 	ldrb	r2, [r0, #8]
  200b84:	e3a03001 	mov	r3, #1	; 0x1
  200b88:	e1a03213 	mov	r3, r3, lsl r2
  200b8c:	e59f20a4 	ldr	r2, [pc, #164]	; 200c38 <.fixed+0xc38>
  200b90:	e5823010 	str	r3, [r2, #16]
  200b94:	e5d0300a 	ldrb	r3, [r0, #10]
  200b98:	e8901004 	ldmia	r0, {r2, ip}
  200b9c:	e3130001 	tst	r3, #1	; 0x1
  200ba0:	e1a030a3 	mov	r3, r3, lsr #1
  200ba4:	e2033001 	and	r3, r3, #1	; 0x1
  200ba8:	e58c2044 	str	r2, [ip, #68]
  200bac:	158c2064 	strne	r2, [ip, #100]
  200bb0:	058c2060 	streq	r2, [ip, #96]
  200bb4:	e3530000 	cmp	r3, #0	; 0x0
  200bb8:	158c2020 	strne	r2, [ip, #32]
  200bbc:	058c2024 	streq	r2, [ip, #36]
  200bc0:	e58c2014 	str	r2, [ip, #20]
  200bc4:	e58c2000 	str	r2, [ip]
  200bc8:	ea000015 	b	200c24 <.fixed+0xc24>
  200bcc:	e5d0300a 	ldrb	r3, [r0, #10]
  200bd0:	e8904004 	ldmia	r0, {r2, lr}
  200bd4:	e35c0004 	cmp	ip, #4	; 0x4
  200bd8:	13a0c000 	movne	ip, #0	; 0x0
  200bdc:	03a0c001 	moveq	ip, #1	; 0x1
  200be0:	e3130001 	tst	r3, #1	; 0x1
  200be4:	e1a03123 	mov	r3, r3, lsr #2
  200be8:	e2033001 	and	r3, r3, #1	; 0x1
  200bec:	e58e2044 	str	r2, [lr, #68]
  200bf0:	158e2064 	strne	r2, [lr, #100]
  200bf4:	058e2060 	streq	r2, [lr, #96]
  200bf8:	e3530000 	cmp	r3, #0	; 0x0
  200bfc:	158e2050 	strne	r2, [lr, #80]
  200c00:	058e2054 	streq	r2, [lr, #84]
  200c04:	e35c0000 	cmp	ip, #0	; 0x0
  200c08:	158e2030 	strne	r2, [lr, #48]
  200c0c:	058e2034 	streq	r2, [lr, #52]
  200c10:	e58e2010 	str	r2, [lr, #16]
  200c14:	e58e2000 	str	r2, [lr]
  200c18:	ea000001 	b	200c24 <.fixed+0xc24>
  200c1c:	e3a00000 	mov	r0, #0	; 0x0
  200c20:	e49df004 	ldr	pc, [sp], #4
  200c24:	e2511001 	subs	r1, r1, #1	; 0x1
  200c28:	e280000c 	add	r0, r0, #12	; 0xc
  200c2c:	1affffb9 	bne	200b18 <PIO_Configure+0xc>
  200c30:	e3a00001 	mov	r0, #1	; 0x1
  200c34:	e49df004 	ldr	pc, [sp], #4
  200c38:	fffffc00 	swinv	0x00fffc00

00200c3c <PIO_Set>:
  200c3c:	e890000c 	ldmia	r0, {r2, r3}
  200c40:	e5832030 	str	r2, [r3, #48]
  200c44:	e1a0f00e 	mov	pc, lr

00200c48 <PIO_Clear>:
  200c48:	e890000c 	ldmia	r0, {r2, r3}
  200c4c:	e5832034 	str	r2, [r3, #52]
  200c50:	e1a0f00e 	mov	pc, lr

00200c54 <PIO_Get>:
  200c54:	e5d03009 	ldrb	r3, [r0, #9]
  200c58:	e2433003 	sub	r3, r3, #3	; 0x3
  200c5c:	e3530001 	cmp	r3, #1	; 0x1
  200c60:	e5903000 	ldr	r3, [r0]
  200c64:	e5900004 	ldr	r0, [r0, #4]
  200c68:	95900038 	ldrls	r0, [r0, #56]
  200c6c:	8590003c 	ldrhi	r0, [r0, #60]
  200c70:	e0100003 	ands	r0, r0, r3
  200c74:	13a00001 	movne	r0, #1	; 0x1
  200c78:	e1a0f00e 	mov	pc, lr

00200c7c <BOARD_GetRemap>:
  200c7c:	e52de004 	str	lr, [sp, #-4]!
  200c80:	e3a0c602 	mov	ip, #2097152	; 0x200000
  200c84:	e59c1000 	ldr	r1, [ip]
  200c88:	e3a0e000 	mov	lr, #0	; 0x0
  200c8c:	e59e3000 	ldr	r3, [lr]
  200c90:	e2812001 	add	r2, r1, #1	; 0x1
  200c94:	e1530002 	cmp	r3, r2
  200c98:	e3a00001 	mov	r0, #1	; 0x1
  200c9c:	11a0000e 	movne	r0, lr
  200ca0:	e58c2000 	str	r2, [ip]
  200ca4:	058c1000 	streq	r1, [ip]
  200ca8:	158c1000 	strne	r1, [ip]
  200cac:	e49df004 	ldr	pc, [sp], #4

00200cb0 <BOARD_RemapRam>:
  200cb0:	e1a0c00d 	mov	ip, sp
  200cb4:	e59f3024 	ldr	r3, [pc, #36]	; 200ce0 <.fixed+0xce0>
  200cb8:	e92dd800 	stmdb	sp!, {fp, ip, lr, pc}
  200cbc:	e24cb004 	sub	fp, ip, #4	; 0x4
  200cc0:	e1a0e00f 	mov	lr, pc
  200cc4:	e1a0f003 	mov	pc, r3
  200cc8:	e20000ff 	and	r0, r0, #255	; 0xff
  200ccc:	e3500001 	cmp	r0, #1	; 0x1
  200cd0:	13a02001 	movne	r2, #1	; 0x1
  200cd4:	13e030ff 	mvnne	r3, #255	; 0xff
  200cd8:	15832000 	strne	r2, [r3]
  200cdc:	e89da800 	ldmia	sp, {fp, sp, pc}
  200ce0:	00200c7c 	eoreq	r0, r0, ip, ror ip

00200ce4 <defaultSpuriousHandler>:
  200ce4:	eafffffe 	b	200ce4 <defaultSpuriousHandler>

00200ce8 <defaultFiqHandler>:
  200ce8:	eafffffe 	b	200ce8 <defaultFiqHandler>

00200cec <defaultIrqHandler>:
  200cec:	eafffffe 	b	200cec <defaultIrqHandler>

00200cf0 <LowLevelInit>:
  200cf0:	e3a02c01 	mov	r2, #256	; 0x100
  200cf4:	e1a0c00d 	mov	ip, sp
  200cf8:	e3e0309f 	mvn	r3, #159	; 0x9f
  200cfc:	e92dd800 	stmdb	sp!, {fp, ip, lr, pc}
  200d00:	e4832010 	str	r2, [r3], #16
  200d04:	e5832000 	str	r2, [r3]
  200d08:	e59f2114 	ldr	r2, [pc, #276]	; 200e24 <.fixed+0xe24>
  200d0c:	e2433e37 	sub	r3, r3, #880	; 0x370
  200d10:	e5832020 	str	r2, [r3, #32]
  200d14:	e24cb004 	sub	fp, ip, #4	; 0x4
  200d18:	e59f2108 	ldr	r2, [pc, #264]	; 200e28 <.fixed+0xe28>
  200d1c:	e5923068 	ldr	r3, [r2, #104]
  200d20:	e3130001 	tst	r3, #1	; 0x1
  200d24:	0afffffb 	beq	200d18 <LowLevelInit+0x28>
  200d28:	e59f30fc 	ldr	r3, [pc, #252]	; 200e2c <.fixed+0xe2c>
  200d2c:	e582302c 	str	r3, [r2, #44]
  200d30:	e59f30f0 	ldr	r3, [pc, #240]	; 200e28 <.fixed+0xe28>
  200d34:	e5933068 	ldr	r3, [r3, #104]
  200d38:	e3130004 	tst	r3, #4	; 0x4
  200d3c:	0afffffb 	beq	200d30 <LowLevelInit+0x40>
  200d40:	e59f20e0 	ldr	r2, [pc, #224]	; 200e28 <.fixed+0xe28>
  200d44:	e5923068 	ldr	r3, [r2, #104]
  200d48:	e3130008 	tst	r3, #8	; 0x8
  200d4c:	0afffffb 	beq	200d40 <LowLevelInit+0x50>
  200d50:	e3a03004 	mov	r3, #4	; 0x4
  200d54:	e5823030 	str	r3, [r2, #48]
  200d58:	e59f20c8 	ldr	r2, [pc, #200]	; 200e28 <.fixed+0xe28>
  200d5c:	e5923068 	ldr	r3, [r2, #104]
  200d60:	e3130008 	tst	r3, #8	; 0x8
  200d64:	0afffffb 	beq	200d58 <LowLevelInit+0x68>
  200d68:	e5923030 	ldr	r3, [r2, #48]
  200d6c:	e3833003 	orr	r3, r3, #3	; 0x3
  200d70:	e5823030 	str	r3, [r2, #48]
  200d74:	e59f30ac 	ldr	r3, [pc, #172]	; 200e28 <.fixed+0xe28>
  200d78:	e5933068 	ldr	r3, [r3, #104]
  200d7c:	e3130008 	tst	r3, #8	; 0x8
  200d80:	0afffffb 	beq	200d74 <LowLevelInit+0x84>
  200d84:	e59f30a4 	ldr	r3, [pc, #164]	; 200e30 <.fixed+0xe30>
  200d88:	e3e02000 	mvn	r2, #0	; 0x0
  200d8c:	e5832124 	str	r2, [r3, #292]
  200d90:	e59f209c 	ldr	r2, [pc, #156]	; 200e34 <.fixed+0xe34>
  200d94:	e5832080 	str	r2, [r3, #128]
  200d98:	e3a01001 	mov	r1, #1	; 0x1
  200d9c:	e2813001 	add	r3, r1, #1	; 0x1
  200da0:	e1a02101 	mov	r2, r1, lsl #2
  200da4:	e20310ff 	and	r1, r3, #255	; 0xff
  200da8:	e59f3088 	ldr	r3, [pc, #136]	; 200e38 <.fixed+0xe38>
  200dac:	e351001e 	cmp	r1, #30	; 0x1e
  200db0:	e5023f80 	str	r3, [r2, #-3968]
  200db4:	9afffff8 	bls	200d9c <LowLevelInit+0xac>
  200db8:	e59f207c 	ldr	r2, [pc, #124]	; 200e3c <.fixed+0xe3c>
  200dbc:	e59f306c 	ldr	r3, [pc, #108]	; 200e30 <.fixed+0xe30>
  200dc0:	e5832134 	str	r2, [r3, #308]
  200dc4:	e3a01000 	mov	r1, #0	; 0x0
  200dc8:	e2813001 	add	r3, r1, #1	; 0x1
  200dcc:	e20310ff 	and	r1, r3, #255	; 0xff
  200dd0:	e59f2058 	ldr	r2, [pc, #88]	; 200e30 <.fixed+0xe30>
  200dd4:	e3a03000 	mov	r3, #0	; 0x0
  200dd8:	e3510007 	cmp	r1, #7	; 0x7
  200ddc:	e5823130 	str	r3, [r2, #304]
  200de0:	9afffff8 	bls	200dc8 <LowLevelInit+0xd8>
  200de4:	e2833001 	add	r3, r3, #1	; 0x1
  200de8:	e5823138 	str	r3, [r2, #312]
  200dec:	e59f304c 	ldr	r3, [pc, #76]	; 200e40 <.fixed+0xe40>
  200df0:	e2822a09 	add	r2, r2, #36864	; 0x9000
  200df4:	e5832004 	str	r2, [r3, #4]
  200df8:	e59f3044 	ldr	r3, [pc, #68]	; 200e44 <.fixed+0xe44>
  200dfc:	e1a0e00f 	mov	lr, pc
  200e00:	e1a0f003 	mov	pc, r3
  200e04:	e59f203c 	ldr	r2, [pc, #60]	; 200e48 <.fixed+0xe48>
  200e08:	e5923000 	ldr	r3, [r2]
  200e0c:	e3c33803 	bic	r3, r3, #196608	; 0x30000
  200e10:	e4823010 	str	r3, [r2], #16
  200e14:	e5923000 	ldr	r3, [r2]
  200e18:	e3c33402 	bic	r3, r3, #33554432	; 0x2000000
  200e1c:	e5823000 	str	r3, [r2]
  200e20:	e89da800 	ldmia	sp, {fp, sp, pc}
  200e24:	00004001 	andeq	r4, r0, r1
  200e28:	fffffc00 	swinv	0x00fffc00
  200e2c:	1048100e 	subne	r1, r8, lr
  200e30:	fffff000 	swinv	0x00fff000
  200e34:	00200ce8 	eoreq	r0, r0, r8, ror #25
  200e38:	00200cec 	eoreq	r0, r0, ip, ror #25
  200e3c:	00200ce4 	eoreq	r0, r0, r4, ror #25
  200e40:	fffffd40 	swinv	0x00fffd40
  200e44:	00200cb0 	streqh	r0, [r0], -r0
  200e48:	fffffd20 	swinv	0x00fffd20

00200e4c <__udivsi3>:
  200e4c:	e2512001 	subs	r2, r1, #1	; 0x1
  200e50:	012fff1e 	bxeq	lr
  200e54:	3a000036 	bcc	200f34 <__udivsi3+0xe8>
  200e58:	e1500001 	cmp	r0, r1
  200e5c:	9a000022 	bls	200eec <__udivsi3+0xa0>
  200e60:	e1110002 	tst	r1, r2
  200e64:	0a000023 	beq	200ef8 <__udivsi3+0xac>
  200e68:	e311020e 	tst	r1, #-536870912	; 0xe0000000
  200e6c:	01a01181 	moveq	r1, r1, lsl #3
  200e70:	03a03008 	moveq	r3, #8	; 0x8
  200e74:	13a03001 	movne	r3, #1	; 0x1
  200e78:	e3510201 	cmp	r1, #268435456	; 0x10000000
  200e7c:	31510000 	cmpcc	r1, r0
  200e80:	31a01201 	movcc	r1, r1, lsl #4
  200e84:	31a03203 	movcc	r3, r3, lsl #4
  200e88:	3afffffa 	bcc	200e78 <__udivsi3+0x2c>
  200e8c:	e3510102 	cmp	r1, #-2147483648	; 0x80000000
  200e90:	31510000 	cmpcc	r1, r0
  200e94:	31a01081 	movcc	r1, r1, lsl #1
  200e98:	31a03083 	movcc	r3, r3, lsl #1
  200e9c:	3afffffa 	bcc	200e8c <__udivsi3+0x40>
  200ea0:	e3a02000 	mov	r2, #0	; 0x0
  200ea4:	e1500001 	cmp	r0, r1
  200ea8:	20400001 	subcs	r0, r0, r1
  200eac:	21822003 	orrcs	r2, r2, r3
  200eb0:	e15000a1 	cmp	r0, r1, lsr #1
  200eb4:	204000a1 	subcs	r0, r0, r1, lsr #1
  200eb8:	218220a3 	orrcs	r2, r2, r3, lsr #1
  200ebc:	e1500121 	cmp	r0, r1, lsr #2
  200ec0:	20400121 	subcs	r0, r0, r1, lsr #2
  200ec4:	21822123 	orrcs	r2, r2, r3, lsr #2
  200ec8:	e15001a1 	cmp	r0, r1, lsr #3
  200ecc:	204001a1 	subcs	r0, r0, r1, lsr #3
  200ed0:	218221a3 	orrcs	r2, r2, r3, lsr #3
  200ed4:	e3500000 	cmp	r0, #0	; 0x0
  200ed8:	11b03223 	movnes	r3, r3, lsr #4
  200edc:	11a01221 	movne	r1, r1, lsr #4
  200ee0:	1affffef 	bne	200ea4 <__udivsi3+0x58>
  200ee4:	e1a00002 	mov	r0, r2
  200ee8:	e12fff1e 	bx	lr
  200eec:	03a00001 	moveq	r0, #1	; 0x1
  200ef0:	13a00000 	movne	r0, #0	; 0x0
  200ef4:	e12fff1e 	bx	lr
  200ef8:	e3510801 	cmp	r1, #65536	; 0x10000
  200efc:	21a01821 	movcs	r1, r1, lsr #16
  200f00:	23a02010 	movcs	r2, #16	; 0x10
  200f04:	33a02000 	movcc	r2, #0	; 0x0
  200f08:	e3510c01 	cmp	r1, #256	; 0x100
  200f0c:	21a01421 	movcs	r1, r1, lsr #8
  200f10:	22822008 	addcs	r2, r2, #8	; 0x8
  200f14:	e3510010 	cmp	r1, #16	; 0x10
  200f18:	21a01221 	movcs	r1, r1, lsr #4
  200f1c:	22822004 	addcs	r2, r2, #4	; 0x4
  200f20:	e3510004 	cmp	r1, #4	; 0x4
  200f24:	82822003 	addhi	r2, r2, #3	; 0x3
  200f28:	908220a1 	addls	r2, r2, r1, lsr #1
  200f2c:	e1a00230 	mov	r0, r0, lsr r2
  200f30:	e12fff1e 	bx	lr
  200f34:	e52de004 	str	lr, [sp, #-4]!
  200f38:	eb000034 	bl	201010 <__div0>
  200f3c:	e3a00000 	mov	r0, #0	; 0x0
  200f40:	e49df004 	ldr	pc, [sp], #4

00200f44 <__umodsi3>:
  200f44:	e2512001 	subs	r2, r1, #1	; 0x1
  200f48:	3a00002c 	bcc	201000 <__umodsi3+0xbc>
  200f4c:	11500001 	cmpne	r0, r1
  200f50:	03a00000 	moveq	r0, #0	; 0x0
  200f54:	81110002 	tsthi	r1, r2
  200f58:	00000002 	andeq	r0, r0, r2
  200f5c:	912fff1e 	bxls	lr
  200f60:	e3a02000 	mov	r2, #0	; 0x0
  200f64:	e3510201 	cmp	r1, #268435456	; 0x10000000
  200f68:	31510000 	cmpcc	r1, r0
  200f6c:	31a01201 	movcc	r1, r1, lsl #4
  200f70:	32822004 	addcc	r2, r2, #4	; 0x4
  200f74:	3afffffa 	bcc	200f64 <__umodsi3+0x20>
  200f78:	e3510102 	cmp	r1, #-2147483648	; 0x80000000
  200f7c:	31510000 	cmpcc	r1, r0
  200f80:	31a01081 	movcc	r1, r1, lsl #1
  200f84:	32822001 	addcc	r2, r2, #1	; 0x1
  200f88:	3afffffa 	bcc	200f78 <__umodsi3+0x34>
  200f8c:	e2522003 	subs	r2, r2, #3	; 0x3
  200f90:	ba00000e 	blt	200fd0 <__umodsi3+0x8c>
  200f94:	e1500001 	cmp	r0, r1
  200f98:	20400001 	subcs	r0, r0, r1
  200f9c:	e15000a1 	cmp	r0, r1, lsr #1
  200fa0:	204000a1 	subcs	r0, r0, r1, lsr #1
  200fa4:	e1500121 	cmp	r0, r1, lsr #2
  200fa8:	20400121 	subcs	r0, r0, r1, lsr #2
  200fac:	e15001a1 	cmp	r0, r1, lsr #3
  200fb0:	204001a1 	subcs	r0, r0, r1, lsr #3
  200fb4:	e3500001 	cmp	r0, #1	; 0x1
  200fb8:	e1a01221 	mov	r1, r1, lsr #4
  200fbc:	a2522004 	subges	r2, r2, #4	; 0x4
  200fc0:	aafffff3 	bge	200f94 <__umodsi3+0x50>
  200fc4:	e3120003 	tst	r2, #3	; 0x3
  200fc8:	13300000 	teqne	r0, #0	; 0x0
  200fcc:	0a00000a 	beq	200ffc <__umodsi3+0xb8>
  200fd0:	e3720002 	cmn	r2, #2	; 0x2
  200fd4:	ba000006 	blt	200ff4 <__umodsi3+0xb0>
  200fd8:	0a000002 	beq	200fe8 <__umodsi3+0xa4>
  200fdc:	e1500001 	cmp	r0, r1
  200fe0:	20400001 	subcs	r0, r0, r1
  200fe4:	e1a010a1 	mov	r1, r1, lsr #1
  200fe8:	e1500001 	cmp	r0, r1
  200fec:	20400001 	subcs	r0, r0, r1
  200ff0:	e1a010a1 	mov	r1, r1, lsr #1
  200ff4:	e1500001 	cmp	r0, r1
  200ff8:	20400001 	subcs	r0, r0, r1
  200ffc:	e12fff1e 	bx	lr
  201000:	e52de004 	str	lr, [sp, #-4]!
  201004:	eb000001 	bl	201010 <__div0>
  201008:	e3a00000 	mov	r0, #0	; 0x0
  20100c:	e49df004 	ldr	pc, [sp], #4

00201010 <__div0>:
  201010:	e12fff1e 	bx	lr

00201014 <PIN_led_utca1>:
  201014:	04000000 fffff600 00000303              ............

00201020 <PIN_led_utca0>:
  201020:	40000000 fffff400 00000302              ...@........

0020102c <PIN_main_nrst>:
  20102c:	00000002 fffff400 00000302              ............

00201038 <PIN_utca_pwron>:
  201038:	00200000 fffff400 00000302              .. .........

00201044 <PIN_flash_serial_sel>:
  201044:	00000040 fffff400 00000402              @...........

00201050 <PIN_dtxd_bootsel>:
  201050:	10000000 fffff400 00010202 18000000     ................
  201060:	fffff400 00000002 63726f66 61735f65     ........force_sa
  201070:	3a61626d 0a642520 00000000 69647473     mba: %d.....stdi
  201080:	3a632e6f 636e6920 73616572 414d2065     o.c: increase MA
  201090:	54535f58 474e4952 5a49535f 000d0a45     X_STRING_SIZE...

002010a0 <_impure_ptr>:
  2010a0:	002010a4                                .. .

002010a4 <r>:
	...
  2010ac:	00000001 00000000 00000000 00000000     ................
	...
