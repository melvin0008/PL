#lang racket

(define  (slow-add x y)
  (letrec ([slow-id (lambda (y z)
                      (if (= z 0) 
                          y
                          (slow-id y (- z 1))))])
    (+ (slow-id x 50000000) y)))

(define (my-mult x y-thunk)
  (cond [(= x 0) 0]
        [(= x 1) (y-thunk)]
        [#t (+ (y-thunk) (my-mult (- x 1) y-thunk))]))