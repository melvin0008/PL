#lang racket
(define (factorial1 x)
  (if (= x 0)
      1
      (* x (factorial1 (- x 1)))))

(define (weird-if e1 e2 e3)
  (if e1 (e2)(e3)))

(define (factorial-strange x)
  (weird-if(= x 0)
           (lambda ()1)
           (lambda ()(* x (factorial-strange(- x 1))))))