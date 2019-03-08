; ModuleID = 'target/test4.c'
source_filename = "target/test4.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 {
entry:
  %retval = alloca i32, align 4
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  %c = alloca i32, align 4
  %d = alloca i32, align 4
  %x = alloca i32, align 4
  store i32 0, i32* %retval, align 4
  %0 = load i32, i32* %a, align 4
  %cmp = icmp sgt i32 %0, 0
  br i1 %cmp, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  %1 = load i32, i32* %c, align 4
  %add = add nsw i32 %1, 10
  store i32 %add, i32* %c, align 4
  br label %if.end5

if.else:                                          ; preds = %entry
  %2 = load i32, i32* %b, align 4
  %cmp1 = icmp sgt i32 %2, 0
  br i1 %cmp1, label %if.then2, label %if.else3

if.then2:                                         ; preds = %if.else
  store i32 3, i32* %c, align 4
  br label %if.end

if.else3:                                         ; preds = %if.else
  %3 = load i32, i32* %c, align 4
  %add4 = add nsw i32 %3, 51
  store i32 %add4, i32* %c, align 4
  br label %if.end

if.end:                                           ; preds = %if.else3, %if.then2
  br label %if.end5

if.end5:                                          ; preds = %if.end, %if.then
  %4 = load i32, i32* %d, align 4
  %cmp6 = icmp sgt i32 %4, 0
  br i1 %cmp6, label %if.then7, label %if.else8

if.then7:                                         ; preds = %if.end5
  store i32 3, i32* %x, align 4
  br label %if.end10

if.else8:                                         ; preds = %if.end5
  %5 = load i32, i32* %x, align 4
  %add9 = add nsw i32 %5, 51
  store i32 %add9, i32* %x, align 4
  br label %if.end10

if.end10:                                         ; preds = %if.else8, %if.then7
  %6 = load i32, i32* %retval, align 4
  ret i32 %6
}

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 7.0.0-3~ubuntu0.18.04.1 (tags/RELEASE_700/final)"}
