; ModuleID = 'target/test4.c'
source_filename = "target/test4.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 {
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  store i32 0, i32* %1, align 4
  %7 = load i32, i32* %2, align 4
  %8 = icmp sgt i32 %7, 0
  br i1 %8, label %9, label %12

; <label>:9:                                      ; preds = %0
  %10 = load i32, i32* %4, align 4
  %11 = add nsw i32 %10, 10
  store i32 %11, i32* %4, align 4
  br label %20

; <label>:12:                                     ; preds = %0
  %13 = load i32, i32* %3, align 4
  %14 = icmp sgt i32 %13, 0
  br i1 %14, label %15, label %16

; <label>:15:                                     ; preds = %12
  store i32 3, i32* %4, align 4
  br label %19

; <label>:16:                                     ; preds = %12
  %17 = load i32, i32* %4, align 4
  %18 = add nsw i32 %17, 51
  store i32 %18, i32* %4, align 4
  br label %19

; <label>:19:                                     ; preds = %16, %15
  br label %20

; <label>:20:                                     ; preds = %19, %9
  %21 = load i32, i32* %5, align 4
  %22 = icmp sgt i32 %21, 0
  br i1 %22, label %23, label %24

; <label>:23:                                     ; preds = %20
  store i32 3, i32* %6, align 4
  br label %27

; <label>:24:                                     ; preds = %20
  %25 = load i32, i32* %6, align 4
  %26 = add nsw i32 %25, 51
  store i32 %26, i32* %6, align 4
  br label %27

; <label>:27:                                     ; preds = %24, %23
  %28 = load i32, i32* %1, align 4
  ret i32 %28
}

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 7.0.0-3~ubuntu0.18.04.1 (tags/RELEASE_700/final)"}
