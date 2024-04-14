use clang_sys::*;
use std::ffi::CString;

fn main() {
    unsafe {
        let index = clang_createIndex(0, 0);
        let tu: *mut CXTranslationUnit = std::ptr::null_mut();
        let err = clang_createTranslationUnit2(
            index,
            CString::new("../clang-c/Index.h").unwrap().as_ptr(),
            tu,
        );
        if err != CXError_Success {
            println!("Error: {}", err);
        }
        clang_disposeTranslationUnit(*tu);
        clang_disposeIndex(index);
    }

    println!("Hello, world!");
}
