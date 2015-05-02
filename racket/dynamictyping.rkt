#lang racket
(define (sum2 xs)
  (if (null? xs)
      0
      (if (number? (car xs))
          (+ (car xs) (sum2 (cdr xs)))
          (if( list? (car xs))
             (+ (sum2 (car xs)) (sum2 (cdr xs)))
             (sum2 (cdr xs))))))
                   