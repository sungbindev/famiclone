; 由 tools/port.mjs 生成：页号常量（库页号从 1 起，对应 start.s 里的 bank_select）
CODE_BANKS       = 5
TOTAL_BANKS      = 10
BANK_IMAGE       = 10                  ; 两份现场缓冲
BANK_DATA_6     = 6                  ; $D000-$D3FF、$D400-$DBFD
BANK_DATA_7     = 7                  ; $DBFE-$E681
BANK_DATA_8     = 8                  ; $E682-$F307
BANK_DATA_9     = 9                  ; $F308-$FFF9
