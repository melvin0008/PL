#lang racket
(define b 3)
(define f (lambda (x)(+ x b)))
(define c (+ b 4))
(define y (f 4))
(set! b 10)
(define z(f 4))
