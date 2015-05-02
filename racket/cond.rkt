#lang racket

(define (sum3 xs)
  (cond [(null? xs) 0]
        [(number? (car xs))(+ (car xs) (sum3 (cdr xs)))]
        [#t (sum3 (cdr xs))]))