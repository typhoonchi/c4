
// c4.c - C 中的四个函数,char、int 和 * 指针类型,if、while、return 和表达式语句
// 2022-04-25 修正
// readNextToken()   词法解析，读下一个token
// parseStatement()  语法解析，分析句子
// parseExpr(level)  表达式解析，分析运算表达式
//main()  主函数 ，参数判断，

#include <stdio.h>   //输入和输出函数库
#include <stdlib.h>  //一些宏和各种通用工具函数库
#include <memory.h>  //内存操作函数库

// 防止编译器编译报错
#ifndef _WIN32 
#include <unistd.h>  //标准符号常量和类型,
#endif

#include <fcntl.h>   //文件控制选项
#define int long long
char *systemWord, //字符指针
*lp,  //在源代码中的当前位置
*data;   //数据指针或者bss指针
int *stack, //
*inBuffer,  // 定义指令缓冲区
*current_id,      // 当前解析的标识符
*symbol,  // 符号表（标识符的简单列表）
tokens,   // 当前 token令牌名称，
tokenValue,     // 当前 token 令牌值
expType,  // 当前表达式类型
location,      // 局部变量偏移
line,     // 当前行号
src,      // 打印源和程序集标志
debug;    // 打印执行的指令

// tokens标记符和类型（操作符排在最后并按优先顺序排列）
enum {
    Num = 128, Fun, Sys, Glo, Loc, Id,
    Char, Else, Enum, If, Int, Return, Sizeof, While,
    Assign, Cond, Lor, Lan, Or, Xor, And, Eq, Ne, Lt, Gt, Le, Ge, Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Brak
};
// 中间代码：汇编操作码，汇编指令
enum {
    LEA, 
    IMM,  // 将某个数放入寄存器 ax 中
    JMP,  //跳转指令
    JSR, 
    BZ, 
    BNZ, 
    ENT, 
    ADJ,
    LEV,
    LI, //将对应地址中的整数载入 ax 中，要求 ax 中存放地址
    LC, //将对应地址中的字符载入 ax 中，要求 ax 中存放地址。
    SI, //将 ax 中的数据作为整数存放入地址中，要求栈顶存放地址。
    SC, //将 ax 中的数据作为字符存放入地址中，要求栈顶存放地址。
    PSH,
    OR, XOR, AND, EQ, NE, LT, GT, LE, GE, SHL, SHR, ADD, SUB, MUL, DIV, MOD,
    OPEN, READ, CLOS, PRTF, MALC, FREE, MSET, MCMP, EXIT
};
enum { CHAR, INT, PTR };
// 标识符偏移（因为我们不能创建一个 ident 结构）,符号表条目的字段索引，`Idsz` 除外。
// `Hash`: 符号名称的哈希值。
// `Name`: 符号名称的字符串地址。
// `Class`: 符号类型：
// - Num：枚举名称。
// - Fun：函数名称。
// - Sys: 系统调用名称。
// - Glo: 全局变量名。
// - Loc：局部变量名称。
// `Type`：关联值类型。例如`CHAR`，`INT`。
// `Val`：关联值。
// `HClass`：`Class` 字段的备份字段。
// `HType`：`Type` 字段的备份字段。
// `HVal`：`Val` 字段的备份字段。
// `Idsz`: 符号表条目大小。

// 标识符偏移量（因为我们无法创建ident结构）
enum { Tokens, Hash, Name, Class, Type, Val, HClass, HType, HVal, Idsz };

// 第一步：词法分析器（读出下一个的token标识符,自动忽略空白字符）
//每个标记格式 (tokens, tokenValue),如100，分析结果是 （number,100）
void readNextToken()
{
    char *pp;
   //用户源代码逐个读取字符：主函数中已经把源代码读入了源代码缓冲区，以`\0`结尾。现在获取当前字符。当前字符不是`\0`，识别到`\0` 结束循环。
    while ((tokens = *systemWord)) {
        //    printf("tokens:[%c]\n",tokens);
        // 指向下一个字符。
        ++systemWord;
        // 如果当前字符是换行符。
    if (tokens == '\n') {
            // 如果是打印源代码行和相应指令的开关
            if (src) {
                // 打印源代码行
                printf("%ld: s%.*s", line, systemWord - lp, lp);
                // 将 `lp` 指向最后一个换行符。
                lp = systemWord;
                // 虽然有打印指令。
                while (inBuffer < stack) {
                    // 打印操作码。
                    printf("%8.4s", &"LEA ,IMM ,JMP ,JSR ,BZ  ,BNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PSH ,"
                        "OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,"
                        "OPEN,READ,CLOS,PRTF,MALC,FREE,MSET,MCMP,EXIT,"[*++inBuffer * 5]);
                    // 如果操作码 <= ADJ，它有操作数。
                    // 打印操作数。
                    if (*inBuffer <= ADJ) printf(" %d\n", *++inBuffer); else printf("\n");
                }
            }
            // 增加行号。
            ++line;
        }
    else if (tokens == '#') {
        //判断 #开头的头文件
            while (*systemWord != 0 && *systemWord != '\n') ++systemWord;
        }
    else if ((tokens >= 'a' && tokens <= 'z') || (tokens >= 'A' && tokens <= 'Z') || tokens == '_') {
        //判断字符串
        pp = systemWord - 1;
        // 当前字符是字母、数字或下划线。
        while ((*systemWord >= 'a' && *systemWord <= 'z') || (*systemWord >= 'A' && *systemWord <= 'Z') || (*systemWord >= '0' && *systemWord <= '9') || *systemWord == '_')
            // 使用当前字符计算哈希值。
            tokens = tokens * 147 + *systemWord++;
        // 将哈希值与字符串长度结合起来。
        tokens = (tokens << 6) + (systemWord - pp);
        // 将 `current_id` 指向符号表。
        current_id = symbol;
        // 当前符号表条目正在使用中。
        while (current_id[Tokens]) {
            // 如果当前符号表条目的哈希值相等且名称相等，则表示该名称之前已经出现过。将令牌类型设置为条目的令牌类型。
            if (tokens == current_id[Hash] && !memcmp((char*)current_id[Name], pp, systemWord - pp)) { tokens = current_id[Tokens]; return; }
            // 指向下一个表条目。
            current_id = current_id + Idsz;
        }
         // 此时，没有找到现有的交易品种名称。 `current_id` 指向第一个未使用的符号表条目。存储名称的字符串地址。
        current_id[Name] = (int)pp;
        // 存储名称的哈希值。
        current_id[Hash] = tokens;
        // 设置令牌类型。
        tokens = current_id[Tokens] = Id;
         //打印token
        //  printf("token:%d\n",tokens);
        return;
    }
    else if (tokens >= '0' && tokens <= '9') {
      // 如果当前字符为数字，则为数字常量。
        // 十进制：如果当前字符不是`0`，则是十进制表示法。  将十进制表示法转换为值。
        if (tokenValue = tokens - '0') { while (*systemWord >= '0' && *systemWord <= '9') tokenValue = tokenValue * 10 + *systemWord++ - '0';
        // 十六进制：如果当前字符是 `0` 并且后面的字符是 `x` 或   `X`，它是十六进制表示法。
        }else if (*systemWord == 'x' || *systemWord == 'X') {
            // 将十六进制表示法转换为值。
            while ((tokens = *++systemWord) && ((tokens >= '0' && tokens <= '9') || (tokens >= 'a' && tokens <= 'f') || (tokens >= 'A' && tokens <= 'F')))
                tokenValue = tokenValue * 16 + (tokens & 15) + (tokens >= 'A' ? 9 : 0);
        }else {
            //八进制：如果当前字符是 `0` 并且后面的字符不是 `x` 或`X`，它是八进制表示法。将八进制表示法转换为值。
            while (*systemWord >= '0' && *systemWord <= '7') tokenValue = tokenValue * 8 + *systemWord++ - '0';
        }
        // 设置令牌类型。
        tokens = Num;
        return;
    }
    else if (tokens == '/') {
        // 如果当前字符为`/`，则为注释或除法运算符。
              // 如果后面的字符是`/`，则为注释。
              if (*systemWord == '/') {
                  // 指向下一个字符。
                  ++systemWord;
                  // 当前字符不是 `\0` 而当前字符不是新队。跳过当前字符。
                  while (*systemWord != 0 && *systemWord != '\n') ++systemWord;
              }
              // 如果后面的字符不是`/`，则为除法运算符。
              else {
                  // 设置令牌类型。
                  tokens = Div;
                  return;
              }
    }
    else if (tokens == '\'' || tokens == '"') {
            //判断双引号和单引号，存储数据缓冲区的当前位置。
            pp = data;
            // 当前字符不是`\0`并且当前字符不是引号字符。
            while (*systemWord != 0 && *systemWord != tokens) {
                // 如果当前字符是 `\`，则它是转义符号或简单的 `\`特点。
                if ((tokenValue = *systemWord++) == '\\') {
                    // 如果后面的字符是`n`，则是换行符，
                    if ((tokenValue = *systemWord++) == 'n') tokenValue = '\n';
                }
                // 如果是字符串常量，则将当前字符复制到数据缓冲区。
                if (tokens == '"') *data++ = tokenValue;
            }
     // 指向下一个字符。
       ++systemWord; 
    // 如果是字符串常量，则使用字符串的地址作为令牌的地址关联值。令牌类型为`"`。如果是字符常量，则使用字符的值作为tokens标记的值关联值。将令牌类型设置为数字常量。
            if (tokens == '"') tokenValue = (int)pp; else tokens = Num;
                return;
            }
    else if (tokens == '=') { if (*systemWord == '=') { ++systemWord; tokens = Eq; } else tokens = Assign; return; }
    else if (tokens == '+') { if (*systemWord == '+') { ++systemWord; tokens = Inc; } else tokens = Add; return; }
    else if (tokens == '-') { if (*systemWord == '-') { ++systemWord; tokens = Dec; } else tokens = Sub; return; }
    else if (tokens == '!') { if (*systemWord == '=') { ++systemWord; tokens = Ne; } return; }
    else if (tokens == '<') { if (*systemWord == '=') { ++systemWord; tokens = Le; } else if (*systemWord == '<') { ++systemWord; tokens = Shl; } else tokens = Lt; return; }
    else if (tokens == '>') { if (*systemWord == '=') { ++systemWord; tokens = Ge; } else if (*systemWord == '>') { ++systemWord; tokens = Shr; } else tokens = Gt; return; }
    else if (tokens == '|') { if (*systemWord == '|') { ++systemWord; tokens = Lor; } else tokens = Or; return; }
    else if (tokens == '&') { if (*systemWord == '&') { ++systemWord; tokens = Lan; } else tokens = And; return; }
    else if (tokens == '^') { tokens = Xor; return; }
    else if (tokens == '%') { tokens = Mod; return; }
    else if (tokens == '*') { tokens = Mul; return; }
    else if (tokens == '[') { tokens = Brak; return; }
    else if (tokens == '?') { tokens = Cond; return; }
    else if (tokens == '~' || tokens == ';' || tokens == '{' || tokens == '}' || tokens == '(' || tokens == ')' || tokens == ']' || tokens == ',' || tokens == ':') return;
   }

}
// 表达式解析。
// `level`：当前运算符优先级。更大的值意味着更高的优先级。
// 运算符优先级（从低到高）：
// Assign  =
// Cond    ?
// Lor     ||
// Lan     &&
// Or      |
// Xor     ^
// And     &
// Eq      ==
// Ne      !=
// Lt      <
// Gt      >
// Le      <=
// Ge      >=
// Shl     <<
// Shr     >>
// Add     +
// Sub     -
// Mul     *
// Div     /
// Mod     %
// Inc     ++
// Dec     --
// Brak    [
void parseExpr(int level)
{
    int paramCount, *d;  // 参数统计
        // 如果当前令牌tokens是不存在，则打印错误并退出程序。
    if (!tokens) { printf("%d: unexpected eof in expression\n", line); exit(-1); }
        // 如果当前tokens标记是数字常量。添加`IMM` 指令将数字的值加载到寄存器中。设置结果值类型为`INT`。
    else if (tokens == Num) { *++stack = IMM; *++stack = tokenValue; readNextToken(); expType = INT; }
        // 如果当前tokens标记是字符串常量。
    else if (tokens == '"') {
        *++stack = IMM; *++stack = tokenValue; readNextToken();
        // 当前tokens标记为字符串常量时，它是相邻字符串常量，例如“ABC”“定义”。在 `readNextToken` 中，字符串的字符已被复制到数据缓冲区。这实现了相邻字符串常量的连接。
        // 读取令牌
        while (tokens == '"') readNextToken();
        // 将 `data` 指向下一个 int 对齐的地址。例如`-sizeof(int)` 是 -4，即 0b11111100。这保证在字符串后至少留下一个 '\0'。设置结果值类型为字符指针。CHAR + PTR = PTR 因为 CHAR 是 0。
        data = (char*)((int)data + sizeof(int) & -sizeof(int)); expType = PTR;
    }
    // 如果当前tokens标记是 `sizeof` 运算符。
    else if (tokens == Sizeof) {
            // 读取令牌。如果当前tokens标记为`(`，则读取tokens标记，否则打印错误并退出程序。
        readNextToken(); if (tokens == '(') readNextToken(); else { printf("%d: open paren expected in sizeof\n", line); exit(-1); }
            // 设置操作数值类型为`INT`。如果当前tokens标记为 `int`，则读取tokens标记。如果当前tokens标记为 `char`，则读取tokens标记，设置操作数值类型为`字符`
        expType = INT; if (tokens == Int) readNextToken(); else if (tokens == Char) { readNextToken(); expType = CHAR; }
            // 当前令牌是`*`，它是指针类型。将 `PTR` 添加到操作数值类型。
        while (tokens == Mul) { readNextToken(); expType = expType + PTR; }
            // 如果当前tokens标记为`)`，则读取tokens标记，否则打印错误并退出程序。
        if (tokens == ')') readNextToken(); else { printf("%d: close paren expected in sizeof\n", line); exit(-1); }
            // 添加`IMM`指令以加载操作数值的大小到寄存器。
        *++stack = IMM; *++stack = (expType == CHAR) ? sizeof(char) : sizeof(int);
            // 设置结果值类型为`INT`。
        expType = INT;
    }
    // 如果当前令牌是标识符。
    else if (tokens == Id) {
        // 存储标识符的符号表入口地址。
        // 读取令牌。
        d = current_id; readNextToken();
        // 如果当前tokens标记为`(`，则为函数调用。
        if (tokens == '(') {
            // 读取 token.
            readNextToken();
               // 参数计数。
            paramCount = 0;
                // 当前令牌不是`)`。解析参数表达式。添加`PSH`指令将参数压入堆栈。增加参数计数。如果当前tokens标记为`,`，则跳过。
            while (tokens != ')') { parseExpr(Assign); *++stack = PSH; ++paramCount; if (tokens == ',') readNextToken(); }
                // Skip `)`
            readNextToken();
                // 如果是系统调用，将系统调用的操作码添加到指令缓冲区。
            if (d[Class] == Sys) *++stack = d[Val];
                // 如果是函数调用，将 `JSR` 操作码和函数地址添加到指令缓冲区。
            else if (d[Class] == Fun) { *++stack = JSR; *++stack = d[Val]; }
                // 否则打印错误信息并退出程序。
            else { printf("%d: bad function call\n", line); exit(-1); }
                // 如果有参数。将 `ADJ` 指令和参数计数添加到指令缓冲区以从函数调用返回后从堆栈中弹出参数。
            if (paramCount) { *++stack = ADJ; *++stack = paramCount; }
                // 设置结果值类型为系统调用或函数的返回类型
            expType = d[Type];
        }
            // 如果是枚举名。添加`IMM` 指令将枚举值加载到寄存器中。设置结果值类型为`INT`。
        else if (d[Class] == Num) { *++stack = IMM; *++stack = d[Val]; expType = INT; }
            // 如果以上都不是，则假设它是一个变量名。
        else {
            // 6S71X如果是局部变量，添加 `LEA` 操作码和局部变量的偏移到指令缓冲区以加载局部变量的地址登记
            if (d[Class] == Loc) { *++stack = LEA; *++stack = location - d[Val]; }
            // 如果是全局变量，添加`IMM`指令加载全局变量要注册的变量地址。
            else if (d[Class] == Glo) { *++stack = IMM; *++stack = d[Val]; }
            // 否则打印错误信息并退出程序。
            else { printf("%d: undefined variable\n", line); exit(-1); }
            // 2WQE9添加`LC`/`LI`指令加载寄存器地址上的值注册。
            *++stack = ((expType = d[Type]) == CHAR) ? LC : LI;
        }
    }
         // 如果当前tokens标记是 `(`，则它是括号中的强制转换或表达式。
    else if (tokens == '(') {
        // 读取令牌。
        readNextToken();
        // 如果当前tokens标记是 `int` 或 `char`，则将其强制转换。
        if (tokens == Int || tokens == Char) {
                // 获取演员的基本数据类型。读取令牌。
            paramCount = (tokens == Int) ? INT : CHAR; readNextToken();
                // 当前令牌是`*`，它是指针类型。将 `PTR` 添加到转换的数据类型。
            while (tokens == Mul) { readNextToken(); paramCount = paramCount + PTR; }
                // 如果当前tokens标记不是`)`，则打印错误并退出程序。
            if (tokens == ')') readNextToken(); else { printf("%d: bad cast\n", line); exit(-1); }
                // 解析转换表达式。使用 `Inc` 以在表达式中只允许 `++`、`--`、`[]` 运算符。
            parseExpr(Inc);
               // 将结果值类型设置为强制转换的数据类型。
            expType = paramCount;
        }
        // 如果当前tokens标记不是 `int` 或 `char`，则是表达式 in括弧。
        else {
               // 解析表达式。
            parseExpr(Assign);
               // 如果当前tokens标记不是`)`，则打印错误并退出程序。
            if (tokens == ')') readNextToken(); else { printf("%d: close paren expected\n", line); exit(-1); }
        }
    }
    // 如果当前tokens标记为 `*`，则为解引用操作符。
    else if (tokens == Mul) {
        // 读取令牌。解析操作数表达式。使用 `Inc` 以在表达式中只允许 `++`、`--`、`[]` 运算符。
        readNextToken();
        parseExpr(Inc);
        // 如果操作数值类型不是指针，则打印错误并退出程序。
        if (expType > INT) expType = expType - PTR; else { printf("%d: bad dereference\n", line); exit(-1); }
        // 添加`LC`/`LI`指令加载寄存器地址上的值注册。
        *++stack = (expType == CHAR) ? LC : LI;
    }

    // 如果当前tokens标记为`&`，则为地址运算符。
    else if (tokens == And) {
        // 读取令牌。解析操作数表达式。使用 `Inc` 以在表达式中只允许 `++`、`--`、`[]` 运算符。
        readNextToken(); parseExpr(Inc);
        // address-of 运算符的操作数应该是一个变量。6S71X 增加了获取变量地址的指令。只需要去掉2WQE9添加的`LC`/`LI`指令即可。如果当前指令是`LC`/`LI`，删除它，否则打??印错误并退出程序。
        if (*stack == LC || *stack == LI) --stack; else { printf("%d: bad address-of\n", line); exit(-1); }
        // 将结果值类型设置为指向当前值类型的指针。
        expType = expType + PTR;
    }
    // 如果当前tokens标记为`!`，则为布尔否定运算符。添加计算 `x == 0` 的指令，因为 `!x` 等价于`x == 0`。设置结果值类型为`INT`。
    else if (tokens == '!') { readNextToken(); parseExpr(Inc); *++stack = PSH; *++stack = IMM; *++stack = 0; *++stack = EQ; expType = INT; }
    // 如果当前tokens标记为`~`，则为按位求逆运算符。 添加指令来计算 `x ^ -1` 因为 `~x` 等价于`x ^ -1`。 设置结果值类型为`INT`。
    else if (tokens == '~') { readNextToken(); parseExpr(Inc); *++stack = PSH; *++stack = IMM; *++stack = -1; *++stack = XOR; expType = INT; }
    // 如果当前tokens标记为`+`，则为一元加法运算符。 读取令牌。解析操作数表达式。设置结果值类型为`INT`。
    else if (tokens == Add) { readNextToken(); parseExpr(Inc); expType = INT; }
    // 如果当前tokens标记为`-`，则为一元减法运算符。
    else if (tokens == Sub) {
        // 读取令牌。添加`IMM`指令来加载数字常量的否定值或`-1`注册。
        readNextToken(); *++stack = IMM;
            // 如果操作数是数字常量，则向指令缓冲区添加负值。如果操作数不是数字常量，则将 `-1` 添加到指令缓冲区。添加`PSH` 指令将寄存器中的 `-1` 压入堆栈。解析操作数表达。添加 `MUL` 指令将堆栈上的 `-1` 乘以寄存器中的操作数值。
        if (tokens == Num) { *++stack = -tokenValue; readNextToken(); }
        else { *++stack = -1; *++stack = PSH; parseExpr(Inc); *++stack = MUL; }
            // 设置结果值类型为`INT`。
        expType = INT;
    }

    // 如果当前tokens标记是前缀递增或递减运算符。
    else if (tokens == Inc || tokens == Dec) {
            // 存储当前令牌类型。读取令牌。解析操作数表达式。
        paramCount = tokens; readNextToken(); parseExpr(Inc);
            // 如果当前指令是 `LC`，则在 `LC` 之前插入一条 `PSH` 指令将寄存器中的变量地址压入堆栈以供 `SC` 使用下面添加了指令。
        if (*stack == LC) { *stack = PSH; *++stack = LC; }
            // 如果当前指令是`LI`，则在`LI`之前插入一条`PSH`指令将寄存器中的变量地址压入堆栈供`SI`使用下面添加了指令。
        else if (*stack == LI) { *stack = PSH; *++stack = LI; }
            // 否则打印错误并退出程序。
        else { printf("%d: bad lvalue in pre-increment\n", line); exit(-1); }
            // 添加`PSH`指令将寄存器中的操作数值压入堆栈供下面添加的 `ADD`/`SUB` 指令使用。
        *++stack = PSH;
            // 添加`IMM`指令来加载增量/减量值到寄存器。
        *++stack = IMM; *++stack = (expType > PTR) ? sizeof(int) : sizeof(char);
            // 添加`ADD`/`SUB`指令来计算结果值。
        *++stack = (paramCount == Inc) ? ADD : SUB;
        // 添加`SC`/`SI`指令将寄存器中的结果值保存到地址保存在堆栈中。
        *++stack = (expType == CHAR) ? SC : SI;
    }
    else 
    { 
        printf("%d: bad expression\n", line); exit(-1); 
    }
     // “优先爬升”或“自上而下运算符优先”方法
    while (tokens >= level) { 
        paramCount = expType;
        if (tokens == Assign) {
            readNextToken();
            if (*stack == LC || *stack == LI) *stack = PSH; else { printf("%d: bad lvalue in assignment\n", line); exit(-1); }
            parseExpr(Assign); *++stack = ((expType = paramCount) == CHAR) ? SC : SI;
        }
        else if (tokens == Cond) {
            readNextToken();
            *++stack = BZ; d = ++stack;
            parseExpr(Assign);
            if (tokens == ':') readNextToken(); else { printf("%d: conditional missing colon\n", line); exit(-1); }
            *d = (int)(stack + 3); *++stack = JMP; d = ++stack;
            parseExpr(Cond);
            *d = (int)(stack + 1);
        }
        else if (tokens == Lor) { readNextToken(); *++stack = BNZ; d = ++stack; parseExpr(Lan); *d = (int)(stack + 1); expType = INT; }
        else if (tokens == Lan) { readNextToken(); *++stack = BZ;  d = ++stack; parseExpr(Or);  *d = (int)(stack + 1); expType = INT; }
        else if (tokens == Or) { readNextToken(); *++stack = PSH; parseExpr(Xor); *++stack = OR;  expType = INT; }
        else if (tokens == Xor) { readNextToken(); *++stack = PSH; parseExpr(And); *++stack = XOR; expType = INT; }
        else if (tokens == And) { readNextToken(); *++stack = PSH; parseExpr(Eq);  *++stack = AND; expType = INT; }
        else if (tokens == Eq) { readNextToken(); *++stack = PSH; parseExpr(Lt);  *++stack = EQ;  expType = INT; }
        else if (tokens == Ne) { readNextToken(); *++stack = PSH; parseExpr(Lt);  *++stack = NE;  expType = INT; }
        else if (tokens == Lt) { readNextToken(); *++stack = PSH; parseExpr(Shl); *++stack = LT;  expType = INT; }
        else if (tokens == Gt) { readNextToken(); *++stack = PSH; parseExpr(Shl); *++stack = GT;  expType = INT; }
        else if (tokens == Le) { readNextToken(); *++stack = PSH; parseExpr(Shl); *++stack = LE;  expType = INT; }
        else if (tokens == Ge) { readNextToken(); *++stack = PSH; parseExpr(Shl); *++stack = GE;  expType = INT; }
        else if (tokens == Shl) { readNextToken(); *++stack = PSH; parseExpr(Add); *++stack = SHL; expType = INT; }
        else if (tokens == Shr) { readNextToken(); *++stack = PSH; parseExpr(Add); *++stack = SHR; expType = INT; }
        else if (tokens == Add) 
        {
            readNextToken(); *++stack = PSH; parseExpr(Mul);
            if ((expType = paramCount) > PTR) { *++stack = PSH; *++stack = IMM; *++stack = sizeof(int); *++stack = MUL; }
            *++stack = ADD;
        }
        else if (tokens == Sub) 
        {
            readNextToken(); *++stack = PSH; parseExpr(Mul);
            if (paramCount > PTR && paramCount == expType) { *++stack = SUB; *++stack = PSH; *++stack = IMM; *++stack = sizeof(int); *++stack = DIV; expType = INT; }
            else if ((expType = paramCount) > PTR) { *++stack = PSH; *++stack = IMM; *++stack = sizeof(int); *++stack = MUL; *++stack = SUB; }
            else *++stack = SUB;
        }
        else if (tokens == Mul) { readNextToken(); *++stack = PSH; parseExpr(Inc); *++stack = MUL; expType = INT; }
        else if (tokens == Div) { readNextToken(); *++stack = PSH; parseExpr(Inc); *++stack = DIV; expType = INT; }
        else if (tokens == Mod) { readNextToken(); *++stack = PSH; parseExpr(Inc); *++stack = MOD; expType = INT; }
        else if (tokens == Inc || tokens == Dec) 
        {
            if (*stack == LC) { *stack = PSH; *++stack = LC; }
            else if (*stack == LI) { *stack = PSH; *++stack = LI; }
            else { printf("%d: bad lvalue in post-increment\n", line); exit(-1); }
            *++stack = PSH; *++stack = IMM; *++stack = (expType > PTR) ? sizeof(int) : sizeof(char);
            *++stack = (tokens == Inc) ? ADD : SUB;
            *++stack = (expType == CHAR) ? SC : SI;
            *++stack = PSH; *++stack = IMM; *++stack = (expType > PTR) ? sizeof(int) : sizeof(char);
            *++stack = (tokens == Inc) ? SUB : ADD;
            readNextToken();
        }
        else if (tokens == Brak) 
        {
            readNextToken(); *++stack = PSH; parseExpr(Assign);
            if (tokens == ']') readNextToken(); else { printf("%d: close bracket expected\n", line); exit(-1); }
            if (paramCount > PTR) { *++stack = PSH; *++stack = IMM; *++stack = sizeof(int); *++stack = MUL; }
            else if (paramCount < PTR) { printf("%d: pointer type expected\n", line); exit(-1); }
            *++stack = ADD;
            *++stack = ((expType = paramCount - PTR) == CHAR) ? LC : LI;
        }
        else { printf("%d: compiler error tokens=%d\n", line, tokens); exit(-1); }
    }
}
//第二步：语法分析
void parseStatement()
{
    int* ax, * b;
        // 如果当前令牌tokens标记为`If`
    if (tokens == If) {
            //读取令牌。
        readNextToken();
            //如果当前令牌不是`（`），则打印错误并退出程序。
        if (tokens == '(') readNextToken(); else { printf("%d: open paren expected\n", line); exit(-1); }
            // 解析测试表达式。
        parseExpr(Assign);
            // 如果当前tokens标记不是`)`，则打印错误并退出程序。
        if (tokens == ')') readNextToken(); else { printf("%d: close paren expected\n", line); exit(-1); }
            // 添加 jump-if-zero 指令 `BZ` 以跳过真正的分支。将 `b` 指向稍后要修补的跳转地址字段。
        *++stack = BZ; b = ++stack;
            // 解析真分支的语句。
        parseStatement();
        if (tokens == Else) {
                // 修补 `b` 指向的跳转地址字段以保存,否则分支。`stack + 3` 不包括下面添加的 `JMP` 指令。在 true 分支后添加 `JMP` 指令以跳过 else分支。将 `b` 指向稍后要修补的跳转地址字段。
            *b = (int)(stack + 3); *++stack = JMP; b = ++stack;
                // 读取令牌。
            readNextToken();
                // 解析 else 分支的语句。
            parseStatement();
        }
        // 修补`b` 指向的跳转地址字段以保存过去的地址if-else 结构。
        *b = (int)(stack + 1);
    }
    else if (tokens == While) {
        readNextToken();   // 读取令牌。
            // 将 `a` 指向循环的测试表达式的地址。
        ax = stack + 1;
            // 如果当前tokens标记不是`(`，则打印错误并退出程序。
        if (tokens == '(') readNextToken(); else { printf("%d: open paren expected\n", line); exit(-1); }
            // 解析测试表达式。
        parseExpr(Assign);
            // 如果当前tokens标记不是`)`，则打印错误并退出程序。
        if (tokens == ')') readNextToken(); else { printf("%d: close paren expected\n", line); exit(-1); }
            // 添加 jump-if-zero 指令 `BZ` 以跳过循环体。将 `b` 指向稍后要修补的跳转地址字段。
        *++stack = BZ; b = ++stack;
            // 解析循环体的语句。
        parseStatement();
            // 添加`JMP`指令跳转到测试表达式。
        *++stack = JMP; *++stack = (int)ax;
            // 修补`b` 指向的跳转地址字段以保存过去的地址循环结构。
        *b = (int)(stack + 1);
    }
    // 如果当前tokens标记是 `return`。
    else if (tokens == Return) {
            // 读取令牌。
        readNextToken();
            // 如果当前tokens标记不是`;`，则为返回表达式。解析返回表达式。
        if (tokens != ';') parseExpr(Assign);
            // 添加`LEV`指令离开函数。
        *++stack = LEV;
            // 如果当前tokens标记为`;`，则读取tokens标记，否则打印错误并退出程序。
        if (tokens == ';') readNextToken(); else { printf("%d: semicolon expected\n", line); exit(-1); }
    }
    // 如果当前令牌是`{`，则是块。
    else if (tokens == '{') {
        readNextToken();
        // 当前令牌不是`}`。解析语句。
        while (tokens != '}') parseStatement();
        // 读取令牌。
        readNextToken();
    }
    // 如果当前tokens标记为`;`，则为语句结束。
    else if (tokens == ';') {
        // 读取令牌。
        readNextToken();
    }
    // 如果当前tokens标记不是上述任何一个，则假定它是表达式。
    else {
        // 解析表达式。
        parseExpr(Assign);
        // 如果当前tokens标记为`;`，则读取tokens标记，否则打印错误并退出程序。
        if (tokens == ';') readNextToken(); else { printf("%d: semicolon expected\n", line); exit(-1); }
    }
}
//入口主函数
int main(int argc, char** argv)
{
    int file,  //文件句柄
    basetType, //基本类型
    expType,  //表达式类型
    poolsize, //程序池尺寸
    * idmain;
    // 定义四个虚拟机中的指针
    int *pc, //虚拟程序计数器，它存放的是一个内存地址，该地址中存放着 下一条 要执行的计算机指令。
    *stackPointer,    //堆栈指针，永远指向当前的栈顶。注意的是由于栈是位于高地址并向低地址增长的，所以入栈时 stackPointer 的值减小。
    *basePointer,    //虚拟基址指针，用于指向栈的某些位置，在调用函数时会使用到它。
    ax,      //虚拟通用寄存器，在虚拟机中，存放一条指令执行后的结果
    cycleCount; //指令周期计数
    int i,
   
    * temp; // 临时缓存
    // 递减 `argc` 以获取命令行参数的数量。增加 `argv` 以指向第一个命令行参数。
    --argc; ++argv;
    // 如果命令行含有参数 `-s`， 打开打印源代码行和对应的开关。
    if (argc > 0 && **argv == '-' && (*argv)[1] == 's') { src = 1; --argc; ++argv; }
    // 如果命令行含有参数 `-d`，打开调试开关。
    if (argc > 0 && **argv == '-' && (*argv)[1] == 'd') { debug = 1; --argc; ++argv; }
    // 如果命令行含有参数 `-h`，打开帮助信息。
    if (argc > 0 && **argv == '-' && (*argv)[1] == 'h') {
        printf("Compiler help command options:\n");
        printf("-help  Help options\n");
        printf("-s  Turn on the printing source code line and the corresponding switch\n");
        printf("-d  Turn on the debugging switch \n"); return -1;
    }
    //如果没有给出源代码文件路径，则打印程序使用情况并退出
    if (argc < 1) { printf("usage: c4 [-s] [-d] file ...\n"); return -1; }
    // 打开源代码文件， 如果失败，打印错误并退出程序。
    if ((file = open(*argv, 0)) < 0) { printf("could not open(%s)\n", *argv); return -1; }
    // 设置缓冲区大小。
    poolsize = 256 * 1024; // 定义一个256K变量
    // 分配符号表，如果失败，打印错误并退出程序。
    if (!(symbol = malloc(poolsize))) { printf("could not malloc(%d) symbolbol area\n", poolsize); return -1; }
    // 分配指令缓冲区， 如果失败，打印错误并退出程序。
    if (!(inBuffer = stack = malloc(poolsize))) { printf("could not malloc(%d) text area\n", poolsize); return -1; }
    // 分配数据缓冲区，如果失败，打印错误并退出程序。
    if (!(data = malloc(poolsize))) { printf("could not malloc(%d) data area\n", poolsize); return -1; }
    // 分配堆栈， 如果失败，打印错误并退出程序。
    if (!(stackPointer = malloc(poolsize))) { printf("could not malloc(%d) stackPointer area\n", poolsize); return -1; }

   // 清除缓冲区。
    memset(symbol, 0, poolsize);
    memset(stack, 0, poolsize);
    memset(data, 0, poolsize);
        // 定义系统关键字和系统调用名称。
    systemWord = "char else enum if int return sizeof while "
        "open read close printf malloc free memset memcmp exit void main";
    // 对于从 `char` 到 `while` 的每个关键字， 调用`readNextToken`来创建符号表条目，
         // 将关键字的tokens标记类型存储在符号表条目的 `Tokens` 字段中。
    i = Char; while (i <= While) { readNextToken(); 
    current_id[Tokens] = i++; }
         // 对于从 `open` 到 `exit` 的每个系统函数名称，调用`readNextToken`来创建符号表条目，将符号表条目的符号类型字段设置为 `Sys`，将符号表条目的关联值类型字段设置为系统调用的返回类型，将符号表条目的关联值字段设置为系统调用的操作码。
    i = OPEN; while (i <= EXIT) { readNextToken(); current_id[Class] = Sys; current_id[Type] = INT; current_id[Val] = i++; }
        // 为 `void` 创建符号表条目。
    readNextToken(); current_id[Tokens] = Char; // handle void type
        // 为 `main` 创建符号表条目。将 `idmain` 指向符号表条目。
    readNextToken(); idmain = current_id; // 跟踪主要信息
   // 分配源代码缓冲区。如果失败，打印错误并退出程序。
    if (!(lp = systemWord = malloc(poolsize))) { printf("could not malloc(%d) source area\n", poolsize); return -1; }
    // 从源文件中读取源代码到源代码缓冲区systemWord。如果失败，打印错误并退出程序。
    if ((i = read(file, systemWord, poolsize - 1)) <= 0) { printf("read() returned %d\n", i); return -1; }
    // 在源缓冲区中的源代码后添加结束tokens标记`\0`。
    systemWord[i] = 0;
    // 关闭源文件。
    close(file);
    // 解析声明
    line = 1; //行号
    // 读 token.
    readNextToken();
    // 当前令牌未输入时结束。
    while (tokens) {
        // 设置结果值类型。
        basetType = INT; // 基类类型
        // 如果当前tokens标记为 `int`，则读取tokens标记。
        if (tokens == Int) readNextToken();
        // 如果当前token为`char`，则读取token，设置结果值类型为`CHAR`。
        else if (tokens == Char) { readNextToken(); basetType = CHAR; }
        // 如果当前tokens标记为 `enum`，则为 enum 定义。
        else if (tokens == Enum) {
            //读 token.
            readNextToken();
            // 如果当前tokens标记不是 `{`，则表示具有枚举类型名称。跳过枚举类型名称。
            if (tokens != '{') readNextToken();
            // 如果当前tokens标记是 `{`。
            if (tokens == '{') {
                // 读取令牌。
                readNextToken();
                // 枚举值从 0 开始。
                i = 0;
                // 当前令牌不是`}`
                while (tokens != '}') {
                    // 当前tokens标记应该是枚举名称。如果当前令牌不是标识符，则打印错误并退出程序。
                    if (tokens != Id) { printf("%d: bad enum identifier %d\n", line, tokens); return -1; }
                    // 读 token
                    readNextToken();
                    // 如果当前tokens标记是赋值运算符。
                    if (tokens == Assign) {
                        // 读 token
                        readNextToken();
                        // 如果当前tokens标记不是数字常量，则打印错误并退出
                         // 程序。
                        if (tokens != Num) { printf("%d: bad enum initializer\n", line); return -1; }
                        // 设置枚举值。
                        i = tokenValue;
                        // 读 token
                        readNextToken();
                    }
                    // `current_id` 指向枚举名称的符号表条目。将符号表条目的符号类型设置为 `Num`。将符号表条目的关联值类型设置为 `INT`。将符号表条目的关联值设置为枚举值。
                    current_id[Class] = Num; current_id[Type] = INT; current_id[Val] = i++;
                    // 如果当前tokens标记为`,`，则跳过。
                    if (tokens == ',') readNextToken();
                }
                // 跳过 `}`.
                readNextToken();
            }
        }
        // 当前tokens标记不是语句结束或块结束。
        while (tokens != ';' && tokens != '}') {
            // 设置值类型。
            expType = basetType;
            // 当前tokens标记为 `*`，但它是指针类型。读取令牌。将 `PTR` 添加到值类型。
            while (tokens == Mul) { readNextToken(); expType = expType + PTR; }
            // 当前tokens标记应该是变量名或函数名。如果当前令牌不是标识符，则打印错误并退出程序
            if (tokens != Id) { printf("%d: bad global declaration\n", line); return -1; }
            // 如果之前已经定义过名称，则打印错误并退出程序。
            if (current_id[Class]) { printf("%d: duplicate global definition\n", line); return -1; }
            readNextToken();
            // 存储变量的数据类型或函数的返回类型。
            current_id[Type] = expType;
            // 如果当前tokens标记为`(`，则为函数定义。
            if (tokens == '(') { // function
            // 存储符号类型。
                current_id[Class] = Fun;
                // 存储函数地址。`+1` 是因为添加指令的代码总是使用 `++stack`。
                current_id[Val] = (int)(stack + 1);
                // 读取令牌。`i` 是参数的索引。
                readNextToken(); i = 0;
                // 解析参数列表。当前令牌不是`)`。
                while (tokens != ')') {
                    // 设置当前参数的数据类型。
                    expType = INT;
                    // 如果当前参数的数据类型为 `int`，则读取tokens标记。
                    if (tokens == Int) readNextToken();
                    // 如果当前参数的数据类型是`char`，读取token，设置数据类型为`CHAR`。
                    else if (tokens == Char) { readNextToken(); expType = CHAR; }
                    // 当前tokens标记为 `*`，但它是指针类型。将 `PTR` 添加到数据类型。
                    while (tokens == Mul) { readNextToken(); expType = expType + PTR; }
                    // 当前tokens标记应该是参数名称。如果当前令牌不是标识符，则打印错误并退出程序。
                    if (tokens != Id) { printf("%d: bad parameter declaration\n", line); return -1; }
                    // 如果之前已经定义了参数名作为参数，则打印出错并退出程序。
                    if (current_id[Class] == Loc) { printf("%d: duplicate parameter definition\n", line); return -1; }
                    // 备份交易品种的`Class`、`Type`、`Val` 字段，因为它们将临时用于参数名称。设置符号类型为局部变量。将关联的值类型设置为参数的数据类型。存储参数的索引。
                    current_id[HClass] = current_id[Class]; current_id[Class] = Loc;
                    current_id[HType] = current_id[Type];  current_id[Type] = expType;
                    current_id[HVal] = current_id[Val];   current_id[Val] = i++;
                    readNextToken();
                    // 如果当前tokens标记为`,`，则跳过。
                    if (tokens == ',') readNextToken();
                }
                readNextToken();
                // 如果当前tokens标记不是函数体的`{`，则打印错误并退出程序。
                if (tokens != '{') { printf("%d: bad function definition\n", line); return -1; }
                // 局部变量偏移。
                location = ++i;
                readNextToken();
                // 当前tokens标记是 `int` 或 `char`，它是变量定义。
                while (tokens == Int || tokens == Char) {
                    // 设置基本数据类型。
                    basetType = (tokens == Int) ? INT : CHAR;
                    readNextToken();
                    // 不满足 while 语句结束。
                    while (tokens != ';') {
                        expType = basetType;
                        // 当前tokens标记为 `*`，但它是指针类型。将 `PTR` 添加到数据类型。
                        while (tokens == Mul) { readNextToken(); expType = expType + PTR; }
                        // 当前tokens标记应该是局部变量名。如果当前令牌不是标识符，则打印错误并退出程序。
                        if (tokens != Id) { printf("%d: bad local declaration\n", line); return -1; }
                        // 如果局部变量名之前已经定义为local变量，打印错误并退出程序。
                        if (current_id[Class] == Loc) { printf("%d: duplicate local definition\n", line); return -1; }
                        // 备份交易品种的`Class`、`Type`、`Val` 字段，因为它们将临时用于局部变量名称。设置符号类型为局部变量。将关联的值类型设置为局部变量的数据类型。存储局部变量的索引。
                        current_id[HClass] = current_id[Class]; current_id[Class] = Loc;
                        current_id[HType] = current_id[Type];  current_id[Type] = expType;
                        current_id[HVal] = current_id[Val];   current_id[Val] = ++i;
                        readNextToken();
                        // 如果当前tokens标记为`,`，则跳过。
                        if (tokens == ',') readNextToken();
                    }
                    readNextToken();
                }
                // 在函数体之前添加 `ENT` 指令。添加局部变量 paramCount 作为操作数。
                *++stack = ENT; *++stack = i - location;
                // 虽然当前tokens标记不是函数体的结尾`}`，解析语句。
                while (tokens != '}') parseStatement();
                // 在函数体之后添加 `LEV` 指令。
                *++stack = LEV;
                // 将 `current_id` 指向符号表。
                current_id = symbol; // unwind symbolbol table locals
                // 当前符号表条目正在使用中。
                while (current_id[Tokens]) {
                    // 如果符号表条目用于函数参数或局部多变的。
                    if (current_id[Class] == Loc) {
                        // 恢复 `Class`、`Type` 和 `Val` 字段的旧值。
                        current_id[Class] = current_id[HClass];
                        current_id[Type] = current_id[HType];
                        current_id[Val] = current_id[HVal];
                    }
                    // 指向下一个符号表条目。
                    current_id = current_id + Idsz;
                }
            }
            // 如果当前tokens标记不是`(`，则不是函数定义，假设它是全局变量定义。
            else {
                // 设置符号类型。
                current_id[Class] = Glo;
                // 存储全局变量的地址。
                current_id[Val] = (int)data;
                // 指向下一个全局变量。
                data = data + sizeof(int);
            }
            // 如果当前tokens标记为`,`，则跳过。
            if (tokens == ',') readNextToken();
        }
        // 读取令牌。
        readNextToken();
    }
    // 将指令指针 `pc` 指向 `main` 函数的地址。如果符号`main`的`Val`字段未设置，则表示`main`函数是未定义，打印错误并退出程序。
    if (!(pc = (int*)idmain[Val])) { printf("main() not defined\n"); return -1; }
    // 如果打印源代码行和对应指令的开关是打开，退出程序。
    if (src) return 0;
    // 设置栈,将帧基指针`basePointer`和栈顶指针`stackPointer`指向栈底。
    basePointer = stackPointer = (int*)((int)stackPointer + poolsize);
    // 将 `EXIT` 指令压入堆栈。注意堆栈向低地址增长，所以在`PSH`指令之后下面添加的是执行，这条`EXIT`指令会被执行退出该程序。
    *--stackPointer = EXIT; // call exit if main returns
    // 将 `PSH` 指令压入堆栈将寄存器中的退出代码压入堆栈在 `main` 函数返回之后。 堆栈上的退出代码将由上面添加了 `EXIT` 指令。将 `t` 指向 `PSH` 指令的地址。
    *--stackPointer = PSH; temp = stackPointer;
    // 将 `main` 函数的第一个参数 `argc` 压入堆栈。
    *--stackPointer = argc;
    // 将 `main` 函数的第二个参数 `argv` 压入堆栈。
    *--stackPointer = (int)argv;
    // 将 `PSH` 指令的地址压入堆栈，以便 `main` 函数
     // 将返回到 `PSH` 指令。
    *--stackPointer = (int)temp;
    // 跑步...指令周期计数。
    cycleCount = 0;
    // 虚拟机运行，循环来执行虚拟机指令。
    while (1) {
        // 获取当前指令。递增指令指针。增加指令周期数。
        i = *pc++; ++cycleCount;
        // 如果调试开关打开。
        if (debug) {
            // 打印汇编指令操作码。
            printf("%d> %.4s", cycleCount,
                &"LEA ,IMM ,JMP ,JSR ,BZ  ,BNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PSH ,"
                "OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,"
                "OPEN,READ,CLOS,PRTF,MALC,FREE,MSET,MCMP,EXIT,"[i * 5]);
            // 如果操作码 <= ADJ，它有操作数。 打印操作数。
            if (i <= ADJ) printf(" %d\n", *pc); else printf("\n");
        }
        // 将帧基指针 `basePointer` 中的基地址添加到操作数。
        if (i == LEA) ax = (int)(basePointer + *pc++);        //本地地址，加载要注册的操作数。
        else if (i == IMM) ax = *pc++;                                         // 全局地址或立即数
        else if (i == JMP) pc = (int*)*pc;                                   // 将第二个操作数中的返回地址压入堆栈。 跳转到第一个操作数中的地址。
        else if (i == JSR) { *--stackPointer = (int)(pc + 1); pc = (int*)*pc; }        // 将第二个操作数中的返回地址压入堆栈。跳转到第一个操作数中的地址。
        else if (i == BZ)  pc = ax ? pc + 1 : (int*)*pc;                      //子程序 如果寄存器值为 0，则跳转到第一个操作数中的地址。
        else if (i == BNZ) pc = ax ? (int*)*pc : pc + 1;                      // 如果寄存器值不为 0，则跳转到第一个操作数中的地址。
        else if (i == ENT) { *--stackPointer = (int)basePointer; basePointer = stackPointer; stackPointer = stackPointer - *pc++; }     // 如果不为零则分支    将`basePointer`中调用者的帧基地址压入堆栈。 将`basePointer`指向被调用者的栈顶。 将栈顶指针 `stackPointer` 减少操作数中的值以保留      // 被调用者的局部变量的空间。
        else if (i == ADJ) stackPointer = stackPointer + *pc++;                                  // 进入子程序  从函数调用返回后从堆栈中弹出参数。
         // 在调用之前将栈顶指针 `stackPointer` 指向调用者的栈顶。将调用者的帧基地址从堆栈弹出到 `basePointer`。旧值被`ENT`指令压入堆栈。
        else if (i == LEV) { stackPointer = basePointer; basePointer = (int*)*stackPointer++; pc = (int*)*stackPointer++; }
        // 离开子程序，在寄存器中的地址上加载 int 值到寄存器。
        else if (i == LI) ax = *(int*)ax;
        // 加载整数，在寄存器中的地址上加载字符值以进行注册。
        else if (i == LC) ax = *(char*)ax;
        // 加载字符，将寄存器中的 int 值保存到堆栈中的地址。
        else if (i == SI)  *(int*)*stackPointer++ =ax;
        // 存储整数，将寄存器中的字符值保存到堆栈中的地址。
        else if (i == SC) ax = *(char*)*stackPointer++ =ax;
        // 存储字符，将寄存器值压入堆栈。
        else if (i == PSH) *--stackPointer =ax;
        // 以下指令采用两个参数。第一个参数在堆栈上。 第二个参数在寄存器中。 结果存入寄存器。
        else if (i == OR) ax = *stackPointer++ |ax;
        else if (i == XOR)ax = *stackPointer++ ^ax;
        else if (i == AND)ax = *stackPointer++ &ax;
        else if (i == EQ) ax = *stackPointer++ ==ax;
        else if (i == NE) ax = *stackPointer++ !=ax;
        else if (i == LT) ax = *stackPointer++ <ax;
        else if (i == GT) ax = *stackPointer++ >ax;
        else if (i == LE) ax = *stackPointer++ <=ax;
        else if (i == GE) ax = *stackPointer++ >=ax;
        else if (i == SHL)ax = *stackPointer++ <<ax;
        else if (i == SHR)ax = *stackPointer++ >>ax;
        else if (i == ADD)ax = *stackPointer++ +ax;
        else if (i == SUB)ax = *stackPointer++ -ax;
        else if (i == MUL)ax = *stackPointer++ *ax;
        else if (i == DIV)ax = *stackPointer++ /ax;
        else if (i == MOD)ax = *stackPointer++ %ax;

//内置函数功能实现：systemWord中定义的内置函数功能实现，下面的指令是系统调用。它们从堆栈中获取参数，就像用户定义的函数一样。注意堆栈向低地址增长，较早推送的参数是在更高的地址。 
     
        // 判断是否为文件打开函数open()，例如。 如果堆栈上有三个参数，则：`stackPointer[2]` 是第一个参数。`stackPointer[1]` 是第二个参数。`*stackPointer` 是第三个参数。打开文件。Arg 1: 要打开的文件路径。参数 2：标志。
        else if (i == OPEN)ax = open((char*)stackPointer[1], *stackPointer);
        //判断是否为文件读取函数read()， 从文件描述符读取到缓冲区。参数 1: 文件描述符。参数 2: 缓冲区指针。参数 3: 要读取的字节数。
        else if (i == READ)ax = read(stackPointer[2], (char*)stackPointer[1], *stackPointer);
        // 判断是否为文件读取函数close()，关闭文件描述符。参数 1: 文件描述符。
        else if (i == CLOS)ax = close(*stackPointer);
        // 打印格式化字符串。因为调用有参数，所以 ADJ 指令应该是添加。 `pc[1]` 获取 ADJ 指令的操作数，即参数。参数 1: 格式字符串。参数 2-7：格式化的值。
        else if (i == PRTF) { temp = stackPointer + pc[1];ax = printf((char*)temp[-1], temp[-2], temp[-3], temp[-4], temp[-5], temp[-6]); }
        //判断是否为内存分配函数malloc()，分配内存块。参数 1: 要分配的字节数。
        else if (i == MALC)ax = (int)malloc(*stackPointer);
        //判断是否为内存释放函数free()，分配的空闲内存块。参数 1: 内存块指针。
        else if (i == FREE) free((void*)*stackPointer);
        // 判断是否为内存释放函数memset()，将内存缓冲区中的每个字节设置为相同的值。参数 1: 缓冲区指针。参数 2：值。参数 3: 要设置的字节数。
        else if (i == MSET)ax = (int)memset((char*)stackPointer[2], stackPointer[1], *stackPointer);
        //判断是否为内存释放函数memcmp()，比较内存缓冲区。参数 1: 第一个缓冲区指针。参数 2: 第二个缓冲区指针。参数 3：要比较的字节数。
        else if (i == MCMP)ax = memcmp((char*)stackPointer[2], (char*)stackPointer[1], *stackPointer);
        //判断是否为内存释放函数exit()，退出程序。参数 1: 退出代码。
        else if (i == EXIT) { printf("exit(%d) cycleCount = %d\n", *stackPointer, cycleCount); return *stackPointer; }
        //您可以在此处扩展其它自定义系统函数功能
        // ....
        //扩展功能结束  
        // 当前指令未知，打印错误并退出程序。
        else { printf("unknown instruction = %d! cycleCount = %d\n", i, cycleCount); return -1; }
    }
}