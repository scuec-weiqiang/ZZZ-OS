CC = gcc
LEX = flex
YACC = bison
CFLAGS = -Wall -O2 -I.

# 输出文件名
OBJS = dtc.o flattree.o fstree.o livetree.o srcpos.o treesource.o util.o \
       dtc-lexer.o dtc-parser.tab.o checks.o data.o

# 最终目标
dtc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

# === Bison (语法分析器) ===
dtc-parser.tab.c dtc-parser.tab.h: dtc-parser.y
	$(YACC) -d -o dtc-parser.tab.c $<

dtc-parser.tab.o: dtc-parser.tab.c
	$(CC) $(CFLAGS) -c -o $@ $<

# === Flex (词法分析器) ===
# 必须等 parser.tab.h 生成后才能生成 lexer.c
# === Flex (词法分析器) ===
dtc-lexer.c: dtc-lexer.l dtc-parser.tab.h
	$(LEX) -o $@ $<

dtc-lexer.o: dtc-lexer.c dtc-parser.tab.h
	$(CC) $(CFLAGS)  -c -o $@ $<

# 其他 C 源文件
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# 清理
clean:
	rm -f dtc *.o dtc-lexer.c dtc-parser.tab.* dtc-parser.output

.PHONY: clean
